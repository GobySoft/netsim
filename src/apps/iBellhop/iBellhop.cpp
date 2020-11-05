// Copyright 2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the NETSIM Binaries.
//
// The NETSIM Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The NETSIM Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#include <complex>
#include <sstream>

#include "iBellhop.h"

#include <boost/bind.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "goby/util/debug_logger.h"
using namespace goby::util::logger;
using goby::util::Colors;
#include "goby/util/as.h"
#include "goby/moos/moos_string.h"
using goby::moos::val_from_string;

const double CiBellhop::pi = 3.14159;

using goby::util::as;

netsim::protobuf::iBellhopConfig CiBellhop::cfg_;
CiBellhop* CiBellhop::inst_ = 0;

CiBellhop* CiBellhop::get_instance()
{
    if(!inst_)
        inst_ = new CiBellhop();
    return inst_;
}

// Construction / Destruction
CiBellhop::CiBellhop()
    : goby::moos::GobyMOOSApp(&cfg_)
{
    if(cfg_.initial_env().water_column_size())
    {
        std::sort(cfg_.mutable_initial_env()->mutable_water_column(0)->mutable_sample()->begin(),
                  cfg_.mutable_initial_env()->mutable_water_column(0)->mutable_sample()->end());
        
        if(!cfg_.initial_env().water_column(0).sample_size() || cfg_.initial_env().water_column(0).sample(0).depth() != 0)
            goby::glog << die << "Must specify depth = 0 value for water column" << std::endl;

    }
    

    if(!cfg_.initial_env().adaptive_info().has_ownship())
        cfg_.mutable_initial_env()->mutable_adaptive_info()->set_ownship(cfg_.common().community());
    
    goby::glog.add_group("system", Colors::lt_magenta, "shell commands");
    goby::glog.add_group("ssp", Colors::lt_blue, "ssp updates");    
    goby::glog.add_group("nr", Colors::green, "contact manager");
    goby::glog.add_group("env", Colors::cyan, "calculate environment");    
    goby::glog.add_group("tl", Colors::blue, "computed TL");
    goby::glog.add_group("request", Colors::yellow, "iBellhopRequest");    
    goby::glog.add_group("response", Colors::magenta, "iBellhopResponse");
    
    subscribe("AVG_SS_VEC", &CiBellhop::handle_ssp_update, this);
    subscribe("SSP_INFO_REQUEST", &CiBellhop::handle_ssp_request, this);
    
    subscribe(cfg_.moos_var_request(), &CiBellhop::handle_request, this);
    subscribe("NODE_REPORT", &CiBellhop::handle_node_report, this);
    subscribe("NAV_X", &CiBellhop::handle_nav_x, this);
    subscribe("NAV_Y", &CiBellhop::handle_nav_y, this);
    subscribe("NAV_DEPTH", &CiBellhop::handle_nav_depth, this);
    subscribe("NAV_SPEED", &CiBellhop::handle_nav_speed, this);
    subscribe("NAV_HEADING", &CiBellhop::handle_nav_heading, this);

    // Publish initial water column svp in SVP_RESPONSE

  std::string serialized_svp_response;
  netsim::SVP::protobuf::Response svp_response;
  svp_response.set_request_id(0);
  svp_response.mutable_water_column()->CopyFrom(cfg_.initial_env().water_column());
  for(int j = 0, m = cfg_.initial_env().water_column_size(); j < m; j++)
    {
      double bathymetry = cfg_.initial_env().bottom().medium().depth();
      if (cfg_.initial_env().water_column(j).has_bathymetry())
	bathymetry = cfg_.initial_env().water_column(j).bathymetry();
      svp_response.mutable_water_column(j)->set_bathymetry(bathymetry);
    }
  serialize_for_moos(&serialized_svp_response,svp_response);
  publish("SVP_RESPONSE",serialized_svp_response);
}

CiBellhop::~CiBellhop()
{}

// "contact=TGT_1,look_ahead_seconds=60,read_shd=false,env.output.type=arrival_times"
void CiBellhop::handle_request(const CMOOSMsg& msg)
{
    netsim::protobuf::iBellhopRequest request;
    parse_for_moos(msg.GetString(), &request);
    goby::glog << group("request") << request.ShortDebugString() << std::endl;

    netsim::bellhop::protobuf::Environment env = cfg_.initial_env();
    netsim::bellhop::protobuf::Environment request_env = request.env();
    

    if(request.water_column_action() == netsim::protobuf::iBellhopRequest::OVERWRITE)
      {
        env.clear_water_column();
      }
    else if(request.water_column_action() == netsim::protobuf::iBellhopRequest::MERGE_SAMPLES)
    {
        if(request_env.water_column_size() && env.water_column_size())
        {
            env.mutable_water_column(0)->MergeFrom(request_env.water_column(0));
            request_env.clear_water_column();
        }
    }
    
    env.MergeFrom(request_env);

    netsim::protobuf::iBellhopResponse response;
    response.set_requestor(msg.GetSource());

    if(request.has_request_number())
        response.set_request_number(request.request_number());
    
    // make contact and ownship lower case
    boost::to_lower(*env.mutable_adaptive_info()->mutable_contact());
    boost::to_lower(*env.mutable_adaptive_info()->mutable_ownship());

    if(env.receivers().number_in_depth() * env.receivers().number_in_range() > cfg_.max_number_of_receivers())
    {
        goby::glog.is_warn() && goby::glog << "number of receivers exceeds maximum of " << cfg_.max_number_of_receivers() << ", ignoring request" << std::endl;
        response.set_success(false);
        goto send_response;
    }
    
    if(!update_positions(env))
    {
        goby::glog << warn << "failed to find source / receiver positions. ignoring request and continuing on" << std::endl;
        response.set_success(false);
        goto send_response;
    }

    {
        calculate_env(env, msg.GetSource(), response.mutable_output_file());
        boost::filesystem::path output_file_abs(response.output_file());
        if(boost::filesystem::exists(output_file_abs))
            response.set_output_file(boost::filesystem::canonical(output_file_abs).native());
        
        if(env.adaptive_info().read_shd())
            read_shd(env, &response, env.adaptive_info().full_shd_matrix());
        response.set_success(true);
    }
    
send_response:
    response.mutable_env()->CopyFrom(env);
    publish_pb(cfg_.moos_var_response(), response);

    goby::glog << group("response") << response.ShortDebugString() << std::endl;
}

void CiBellhop::handle_ssp_update(const CMOOSMsg& msg)
{
     const std::string& s = msg.GetString();
     goby::glog << group("ssp") <<  s << std::endl;

    std::string depth, speed;
    val_from_string(depth, s, "depth");
    val_from_string(speed, s, "c");    

    std::vector<std::string> depths, speeds;
    boost::split(depths, depth, boost::is_any_of(","));
    boost::split(speeds, speed, boost::is_any_of(","));

    if(depths.size() != speeds.size())
    {
        goby::glog << group("ssp") << warn << "depth vec size != c vec size" << std::endl;
        return;
    }

    for(size_t i = 0, n = depths.size(); i < n; ++i)
    {
        if(speeds[i] != "0" && speeds[i] != "")
        {
            double depth = as<double>(depths[i]);
            double cp = as<double>(speeds[i]);
            
            
            netsim::bellhop::protobuf::Environment::WaterColumn::SSPSample* ssp =
                find_sample(cfg_.mutable_initial_env(), depth);
            ssp->set_cp(cp);
            ssp->set_depth(depth);
	    // force eofs to zero for updated ssp
	    for (int j = 0; j < ssp->eof_size(); j++)
	      ssp->set_eof(j,0);
        }
    }

    std::sort(cfg_.mutable_initial_env()->mutable_water_column(0)->mutable_sample()->begin(),
              cfg_.mutable_initial_env()->mutable_water_column(0)->mutable_sample()->end());
    
}

void CiBellhop::handle_ssp_request(const CMOOSMsg& msg)
{
  const std::string& s = msg.GetString();
  goby::glog << group("ssp") <<  s << std::endl;
  
  std::string depth;
  val_from_string(depth, s, "depth");
  
  std::vector<std::string> depths, speeds;
  boost::split(depths, depth, boost::is_any_of(","));
  speeds.resize(depths.size());
  
  std::stringstream ss;
  for(size_t i = 0, n = depths.size(); i < n; ++i)
    {
      double dep = as<double>(depths[i]);
      double cp = get_ssp_value(cfg_.mutable_initial_env(), dep);
      if (i > 0)
	ss << ",";
      ss << cp ;
    }
  std::string ssp_resp = msg.GetString() + ",c={" + ss.str() + "}";
  publish(std::string("SSP_INFO_RESPONSE"), ssp_resp);
}

netsim::bellhop::protobuf::Environment::WaterColumn::SSPSample* CiBellhop::find_sample(netsim::bellhop::protobuf::Environment* env, double depth)
{

    for(int i = 0, n = env->water_column(0).sample_size(); i < n; ++i)
    {
        if(depth == env->water_column(0).sample(i).depth())
            return env->mutable_water_column(0)->mutable_sample(i);
    }
    
    return env->mutable_water_column(0)->add_sample();
    
}

double CiBellhop::get_ssp_value(netsim::bellhop::protobuf::Environment* env, double depth)
{

  int m_coef = env->water_column(0).eof_coef_size();
  std::vector<double> eof_coef;
  eof_coef.resize(m_coef);
  for (int j = 0; j < m_coef; j++)
    eof_coef[j] = env->water_column(0).eof_coef(j);

  for(int i = 0, n = env->water_column(0).sample_size(); i < n-1; ++i)
    {
      if(depth >= env->water_column(0).sample(i).depth() && 
	 depth <= env->water_column(0).sample(i+1).depth())
	{
	  double cp    = env->water_column(0).sample(i).cp();  
	  int m = env->water_column(0).sample(i).eof_size();
	  double dep_up = env->water_column(0).sample(i).depth();  
	  double ssp_up = cp ;
	  for (int j = 0; j < std::min(m,m_coef); j++)
	    ssp_up += env->water_column(0).sample(i).eof(j) * eof_coef[j];

	  cp = env->water_column(0).sample(i+1).cp();  
	  double dep_dn = env->water_column(0).sample(i+1).depth();  
	  double ssp_dn = cp ;
	  for (int j = 0; j < std::min(m,m_coef); j++)
	    ssp_dn += env->water_column(0).sample(i+1).eof(j) * eof_coef[j];

	  double ssp = ssp_up + 
	    (depth - dep_up) * (ssp_dn - ssp_up) / (dep_dn - dep_up);
	  return(ssp);
	}
    }
  
  return(0.0);
  
}

netsim::bellhop::protobuf::Environment::WaterColumn::SSPSample* CiBellhop::get_sample(netsim::bellhop::protobuf::Environment* env, int i)
{
  return env->mutable_water_column(0)->mutable_sample(i);
}
    
void CiBellhop::handle_node_report(const CMOOSMsg& msg)
{
    const std::string& s = msg.GetString();    

    std::string name;
    val_from_string(name, s, "NAME");
    boost::to_lower(name);
    
    NodeReport& nr = node_reports_[name];
    
    val_from_string(nr.x, s, "X");
    val_from_string(nr.y, s, "Y");
    if(!val_from_string(nr.depth, s, "DEPTH"))
        val_from_string(nr.depth, s, "DEP");
    val_from_string(nr.speed, s, "SPD");
    val_from_string(nr.heading, s, "HDG");
    if(!val_from_string(nr.time, s, "UTC_TIME"))
        val_from_string(nr.time, s, "TIME");
    
    goby::glog << group("nr") << "got update for collaborator " << name << " - x: " << nr.x << ", y: " << nr.y << " and depth: " << nr.depth <<  std::endl;

}

bool CiBellhop::update_positions(netsim::bellhop::protobuf::Environment& env)
{
    // do we need to do anything?
    if(!env.adaptive_info().auto_receiver_ranges() &&
       !env.adaptive_info().auto_source_depth())
        return true;

    const std::string& other_name = env.adaptive_info().contact();
    const std::string& self_name = env.adaptive_info().ownship();
        
    int look_ahead_seconds = env.adaptive_info().look_ahead_seconds();
        
    if(!node_reports_.count(other_name))
    {
        goby::glog << group("nr") << warn << "no known collaborator '"<< other_name << "'" << std::endl;
        return false;
    }
    else if(!node_reports_.count(self_name))
    {
        goby::glog << group("nr") << warn << "no known ownship '"<< self_name << "'" << std::endl;
        return false;
    }

    NodeReport self_nav = node_reports_[self_name];
    NodeReport other_nav = node_reports_[other_name];
    
    if(cfg_.extrapolate_nav())
    {
        double dt = MOOSTime() - other_nav.time;
        double dr = other_nav.speed * dt;
        double dx = dr*std::cos((90 - other_nav.heading) * pi/180); 
        double dy = dr*std::sin((90 - other_nav.heading) * pi/180);
        other_nav.x += dx;
        other_nav.y += dy;
    }
    
    double dx = other_nav.x - self_nav.x ;
    double dy = other_nav.y - self_nav.y;
    double r = std::sqrt(dx*dx + dy*dy);
    double other_nav_bearing = atan2(dy,dx);

    // HS: Publish current collaborator range, speed and depth 
    // for use by adaptive behaviors
    publish(std::string(boost::to_upper_copy(other_name) + "_RANGE"), r);
    publish(std::string(boost::to_upper_copy(other_name) + "_DEPTH"), other_nav.depth);

    double self_nav_theta = (90 - self_nav.heading) * pi/180;
    double other_nav_theta = (90 - other_nav.heading) * pi/180;

    double opening_speed = other_nav.speed * cos(other_nav_theta-other_nav_bearing) -
        self_nav.speed * cos(self_nav_theta - other_nav_bearing);
    publish(std::string(boost::to_upper_copy(other_name) + "_RATE"), opening_speed);

    // where do we expect to be in the next `look_ahead_seconds`?
    double self_nav_future_x = self_nav.x + self_nav.speed*cos(self_nav_theta) *
        look_ahead_seconds;
    double self_nav_future_y = self_nav.y + self_nav.speed*sin(self_nav_theta) *
        look_ahead_seconds;

    double other_nav_future_x = other_nav.x + other_nav.speed*cos(other_nav_theta) *
        look_ahead_seconds;
    double other_nav_future_y = other_nav.y + other_nav.speed*sin(other_nav_theta) *
        look_ahead_seconds;
    double future_dx = self_nav_future_x - other_nav_future_x;
    double future_dy = self_nav_future_y - other_nav_future_y;

    double future_r = std::sqrt(future_dx*future_dx + future_dy*future_dy);

    goby::glog << group("env") << self_name << ": " << self_nav << std::endl;
    goby::glog << group("env") << other_name << ": " << other_nav << std::endl;
    goby::glog << group("env") << self_name << " to " << other_name << ": current range is " << r << " meters " << std::endl;
    goby::glog << group("env") << self_name << " to " << other_name << ": delta range for next " << look_ahead_seconds << " seconds is " << future_r-r << " meters " << std::endl;
    

    if(env.adaptive_info().auto_source_depth())
        env.mutable_sources()->mutable_first()->set_depth(other_nav.depth);

    if(env.adaptive_info().auto_receiver_ranges())
    {
        env.mutable_receivers()->set_number_in_range(std::ceil(std::abs(future_r - r)/env.adaptive_info().auto_receiver_ranges_delta()));

        // env.receivers().set_number_in_range(std::ceil(std::abs(future_r - r)));
        
        //env.receivers().first.set_depth(0); // surface
        //env.receivers().last.set_depth(env.bottom().depth); // bottom
        
        if(future_r >= r)
        {
            env.mutable_receivers()->mutable_first()->set_range(r);
            env.mutable_receivers()->mutable_last()->set_range(future_r);
        }
        else
        {
            env.mutable_receivers()->mutable_first()->set_range(future_r);
            env.mutable_receivers()->mutable_last()->set_range(r);
        }
    }
    
    return true;
}

void CiBellhop::calculate_env(netsim::bellhop::protobuf::Environment& env, const std::string& requestor, std::string* output_file)
{
    const std::string& other_name = env.adaptive_info().contact();
    const std::string& self_name = env.adaptive_info().ownship();

    // bellhop env file    
    std::string file = cfg_.output_env_dir() + "/" + boost::replace_all_copy(requestor.substr(0, 20),":", "_") +
        "_" + other_name  + "_to_" + self_name + "_" +
      to_iso_string(boost::posix_time::second_clock::universal_time()) + ".env";
    env_out_.open(file.c_str());

    // bellhop ssp file
    std::string file_ssp = file.substr(0,file.size()-4) + ".ssp";
    ssp_out_.open(file_ssp.c_str());
    // bellhop bty file    
    std::string file_bty = file.substr(0,file.size()-4) + ".bty";
    bty_out_.open(file_bty.c_str());
    // bellhop trc file    
    std::string file_trc = file.substr(0,file.size()-4) + ".trc";
    trc_out_.open(file_trc.c_str());
    // bellhop brc file    
    std::string file_brc = file.substr(0,file.size()-4) + ".brc";
    brc_out_.open(file_brc.c_str());

    
    if(!env_out_.is_open())
        goby::glog << die << "cannot write to " << cfg_.output_env_dir() << std::endl;

    try 
      {
	netsim::bellhop::Environment::output_env(&env_out_, 
					 &ssp_out_,
					 &bty_out_,
					 &trc_out_,
					 &brc_out_,
					 env);
      }
    catch(std::exception& e) 
      {
	goby::glog << "Failed to create Bellhop file: " << e.what() << std::endl;
	return;
      }

    goby::glog << group("env") << "writing env file to " << file << std::endl;
    goby::glog << group("env") << env.DebugString() << std::endl;
        
    env_out_.close();
    ssp_out_.close();
    bty_out_.close();
    trc_out_.close();
    brc_out_.close();

    goby::glog << group("env") << "running bellhop.exe" << std::endl;
    std::string bellhop_command = "bellhop.exe " + file.substr(0,file.size()-4);
    goby::glog << group("system") << bellhop_command << std::endl;    
    system(bellhop_command.c_str());

    // notify others of location of generated file
    switch(env.output().type())
    {
        case netsim::bellhop::protobuf::Environment::Output::ARRIVAL_TIMES:
            *output_file = file.substr(0,file.size()-4) + ".arr";
            publish(std::string(boost::to_upper_copy(other_name) + "_ARR_FILE"),
                    *output_file);
            break;
            
        case netsim::bellhop::protobuf::Environment::Output::EIGENRAYS:
        case netsim::bellhop::protobuf::Environment::Output::RAYS:
            *output_file = file.substr(0,file.size()-4) + ".ray";
            publish(std::string(boost::to_upper_copy(other_name) + "_RAY_FILE"),
                    *output_file);
            break;
            
        case netsim::bellhop::protobuf::Environment::Output::COHERENT_PRESSURE:
        case netsim::bellhop::protobuf::Environment::Output::INCOHERENT_PRESSURE:
        case netsim::bellhop::protobuf::Environment::Output::SEMICOHERENT_PRESSURE:
        {
            *output_file = file.substr(0,file.size()-4) + ".shd";
            publish(std::string(boost::to_upper_copy(other_name) + "_SHD_FILE"),
                    *output_file);
            
            std::string create_asc_command = "cp " + *output_file + " " + cfg_.output_env_dir() + "/SHDFIL";
            goby::glog << group("system") << create_asc_command << std::endl;                    
            system(create_asc_command.c_str());
        }
        break;
    }

}

void CiBellhop::read_shd(netsim::bellhop::protobuf::Environment& env,
                         netsim::protobuf::iBellhopResponse* response,
                         bool full_shd_matrix)
{
//    const std::string& other_name = env.adaptive_info().contact();
    netsim::protobuf::iBellhopResponse::TLAveragedInRange* avg_tl = response->mutable_avg_tl();
    //    netsim::protobuf::iBellhopResponse::TLvsRangeDepth* tl_matrix = response->mutable_tl_matrix();

    
    std::string to_asc_command = "cd " + cfg_.output_env_dir() + "; toasc.exe 1> /dev/null";
    goby::glog << group("system") << to_asc_command << std::endl;
    system(to_asc_command.c_str());
    
    std::string asc_file_path = cfg_.output_env_dir() + "/ASCFIL";
    std::ifstream asc_file;
    asc_file.open(asc_file_path.c_str());

    if(asc_file.is_open())
    {
        std::string line;
        std::getline(asc_file, line); //BELLHOP- GLINT10 03Aug2010_ctd12
        std::getline(asc_file, line);//  'rectilin  '   0.0000000       0.0000000   

        double freq;
        asc_file >> freq;
        int unused;
        asc_file >> unused;
        asc_file >> unused;
        int num_depths;
        asc_file >> num_depths;
        int num_ranges;
        asc_file >> num_ranges;
        double unused2;
        asc_file >> unused2;
        asc_file >> unused2;
        asc_file >> unused2;

        netsim::bellhop::TLMatrix tl_matrix_obj;
        
        std::vector<float>& depths = tl_matrix_obj.depths;
        for(int i = 0; i < num_depths; ++i)
        {
            float d;
            asc_file >> d;
            depths.push_back(d);
        }

        std::vector<float>& ranges = tl_matrix_obj.ranges;
        for(int i = 0; i < num_ranges; ++i)
        {
            float d;
            asc_file >> d;
            ranges.push_back(d);
        }

        // over depth
        std::vector< std::vector<float> >& TL_matrix = tl_matrix_obj.tl;
        for(int i = 0; i < num_depths; ++i)
        {
            std::vector<float> TL_vector;
            for(int j = 0; j < num_ranges; ++j)
            {
                double real, imag;
                asc_file >> real;
                asc_file >> imag;
                std::complex<double> pressure(real, imag);
                    
                float TL = -20*log10(abs(pressure));
                TL_vector.push_back(TL);
            }
            TL_matrix.push_back(TL_vector);
        }

        if(full_shd_matrix)
        {
            // serialize
            std::stringstream oss;
            boost::archive::binary_oarchive oa(oss);
            oa << tl_matrix_obj;
            response->set_serialized_tl_matrix(oss.str());
        }
	
        
        for(int i = 0; i < num_depths; ++i)
	  {
            std::vector<double> TL_vector;
            double avg_intensity = 0;
            for(int j = 0; j < num_ranges; ++j)
            {
	      avg_intensity += pow(10.0, TL_matrix[i][j]/10);
            }
            netsim::protobuf::iBellhopResponse::TLAveragedInRange::TLSample* sample = avg_tl->add_sample();
            sample->set_depth(depths[i]);
            sample->set_tl(10*log10(avg_intensity / num_ranges));
	  }            
    }
}
    



void CiBellhop::handle_nav_depth(const CMOOSMsg& msg)
{
    node_reports_[cfg_.initial_env().adaptive_info().ownship()].depth = msg.GetDouble();

    // post interpolated sound speed from environment, if desired
    if(cfg_.has_local_sound_speed_var())
    {
      // use last merged environment
      double depth = msg.GetDouble();
      
      double cp = get_ssp_value(cfg_.mutable_initial_env(), depth);

      publish(cfg_.local_sound_speed_var(), cp);
    }
} 
