#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"
#include "goby/middleware/io/line_based/serial.h"

#include "netsim/messages/groups.h"
#include "netsim/messages/config_request.pb.h"
#include "netsim/tcp/tcp_server.h"
#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"
#include "lamss/lib_bellhop/iBellhop_messages.pb.h"
#include "netsim/messages/manager_config.pb.h"
#include "netsim/messages/env_bellhop_req.pb.h"

using namespace goby::util::logger;
using goby::glog;

class NetSimManager : public goby::zeromq::MultiThreadApplication<NetSimManagerConfig>
{
public:
    NetSimManager();

private:
    void loop() override;
    void process_configuration();
    void process_request(const NetSimManagerRequest& r, const boost::asio::ip::tcp::endpoint& ep);

    void handle_impulse_request(const ImpulseRequest& req);
    void handle_performance_request(const ObjFuncRequest& req);
    void handle_bellhop_request(const iBellhopRequest& req);

    void write_gps_out(const NavUpdate& nav_update);
    
private:
    // maps modem_tcp_port to configuration
    std::map<int, NetSimManagerConfig::SimEnvironmentPair> env_cfg_;
    // maps environment ID to bounds
    std::map<int, NetSimManagerConfig::EnvironmentBounds> env_bounds_;

    boost::asio::io_service io_;
    netsim::tcp_server server_{io_, short(61999)};

};



int main(int argc, char* argv[])
{ return goby::run<NetSimManager>(argc, argv); }


NetSimManager::NetSimManager() :
    goby::zeromq::MultiThreadApplication<NetSimManagerConfig>(10*boost::units::si::hertz)
{
    process_configuration();

    server_.read_callback<NetSimManagerRequest>(
	[this](const NetSimManagerRequest& r, const boost::asio::ip::tcp::endpoint& ep)
	{ process_request(r, ep); });

    interprocess().subscribe<groups::impulse_request, ImpulseRequest>(
	[this](const ImpulseRequest& req)
	{ handle_impulse_request(req); });

    interprocess().subscribe<groups::performance_request, ObjFuncRequest>(
	[this](const ObjFuncRequest& req)
	{ handle_performance_request(req); });

    interprocess().subscribe<groups::config_request, ConfigRequest>(
	[this](const ConfigRequest& req)
	{
	    if(req.subsystem() == ConfigRequest::MANAGER)
		interprocess().publish<groups::configuration>(cfg());
	});


    interprocess().subscribe<groups::bellhop_request, iBellhopRequest>(
	[this](const iBellhopRequest& req)
	{ handle_bellhop_request(req);} );
    
    for(const auto& gps_out : cfg().gps_out())
    {
	// use modem_tcp_port as thread index
	launch_thread<goby::middleware::io::SerialThreadLineBased<groups::gps_line_in, groups::gps_line_out, goby::middleware::io::PubSubLayer::INTERTHREAD, goby::middleware::io::PubSubLayer::INTERTHREAD>>(static_cast<int>(gps_out.modem_tcp_port()), gps_out.serial());
    }
}

void NetSimManager::loop()
{
    io_.poll();
}

void NetSimManager::process_configuration()
{
    for(const auto& bound : cfg().env_bounds())
    {
	if(env_bounds_.count(bound.environment_id()))
	    glog.is(DIE) && glog << "Environment " << bound.environment_id() << " has bounds specified multiple times. " << std::endl;
	env_bounds_[bound.environment_id()] = bound;
    
    }

    for(const auto& sim_env : cfg().sim_env_pair())
    {
	if(env_cfg_.count(sim_env.modem_tcp_port()))
	    glog.is(DIE) && glog << "Modem TCP Port " << sim_env.modem_tcp_port() << " specified multiple times." << std::endl;

	if(env_bounds_.count(sim_env.environment_id()) == 0)
	    glog.is(DIE) && glog << "No bounds given for environment " << sim_env.environment_id() << std::endl;
    
	env_cfg_[sim_env.modem_tcp_port()] = sim_env;
    }
}

void NetSimManager::process_request(const NetSimManagerRequest& req, const boost::asio::ip::tcp::endpoint& ep)
{
    glog.is(DEBUG1) && glog << "Received request from: " << ep << ": " << req.ShortDebugString() << std::endl;

    NetSimManagerResponse resp;
    resp.set_request_id(req.id());

    NetSimManagerResponse::Status status = NetSimManagerResponse::UPDATE_ACCEPTED;

    for(const auto& nav : req.nav())
    {
	auto env_cfg_it = env_cfg_.find(nav.modem_tcp_port());
	if(env_cfg_it == env_cfg_.end())
	{
	    status = NetSimManagerResponse::UPDATE_FAILED_INVALID_MODEM_TCP_PORT;
	    break;
	}
	const auto& env_cfg = env_cfg_it->second;
	
	// check source address
	if(ep.address().to_string() != env_cfg.endpoint_ip_address())
	{
	    status = NetSimManagerResponse::UPDATE_FAILED_INVALID_SOURCE_ADDRESS;
	    break;
	}		    

	const auto& bound = env_bounds_.at(env_cfg.environment_id());

	if(nav.lat() < bound.lat_min() || nav.lat() > bound.lat_max() ||
	   nav.lon() < bound.lon_min() || nav.lon() > bound.lon_max() ||
	   nav.depth() < bound.depth_min() || nav.depth() > bound.depth_max())
	{
	    status = NetSimManagerResponse::UPDATE_FAILED_OUT_OF_DEFINED_REGION;
	    break;
	}
	
	EnvironmentNavUpdate env_nav_update;
	env_nav_update.set_environment_id(env_cfg.environment_id());
	*env_nav_update.mutable_nav() = nav;

	if(env_nav_update.IsInitialized())
	{
	    interprocess().publish<groups::env_nav_update>(env_nav_update);
	    write_gps_out(env_nav_update.nav());
	}
	else
	{
	    glog.is(WARN) && glog << "Uninitialized EnvironmentNavUpdate: " << env_nav_update.DebugString() << std::endl;
	}
    }


    for(const auto& stat : req.stats())	
    {
	interprocess().publish<groups::receive_stats>(stat);
    }

    
    resp.set_status(status);
    
    server_.write(resp, ep);
}

void NetSimManager::handle_impulse_request(const ImpulseRequest& req)
{
    glog.is(DEBUG1) && glog << "Received ImpulseRequest: " << req.ShortDebugString() << std::endl;

    int source = goby::util::as<int>(req.source());

    auto env_cfg_it = env_cfg_.find(source);
    if(env_cfg_it == env_cfg_.end())
    {
	glog.is(WARN) && glog << "Unknown source modem_tcp_port: " << source << " (string: " << req.source() << ")" << std::endl;
	return;
    }
    const auto& env_cfg = env_cfg_it->second;

    EnvironmentImpulseRequest env_req;
    env_req.set_environment_id(env_cfg.environment_id());
    *env_req.mutable_req() = req;
    interprocess().publish<groups::env_impulse_req>(env_req);
}

void NetSimManager::handle_performance_request(const ObjFuncRequest& req)
{
    glog.is(DEBUG1) && glog << "Received ObjFuncRequest: " << req.ShortDebugString() << std::endl;

    int source = goby::util::as<int>(req.contact());

    auto env_cfg_it = env_cfg_.find(source);
    if(env_cfg_it == env_cfg_.end())
    {
	glog.is(WARN) && glog << "Unknown source modem_tcp_port: " << source << " (string: " << req.contact() << ")" << std::endl;
	return;
    }
    const auto& env_cfg = env_cfg_it->second;

    EnvironmentObjFuncRequest env_req;
    env_req.set_environment_id(env_cfg.environment_id());
    *env_req.mutable_req() = req;
    interprocess().publish<groups::env_performance_req>(env_req);
}


void NetSimManager::handle_bellhop_request(const iBellhopRequest& req)
{
    glog.is(DEBUG1) && glog << "Received iBellhopRequest: " << req.ShortDebugString() << std::endl;

    int source = goby::util::as<int>(req.env().adaptive_info().ownship());

    auto env_cfg_it = env_cfg_.find(source);
    if(env_cfg_it == env_cfg_.end())
    {
	glog.is(WARN) && glog << "Unknown source modem_tcp_port: " << source << " (string: " << req.env().adaptive_info().ownship() << ")" << std::endl;
	return;
    }
    const auto& env_cfg = env_cfg_it->second;

    EnvironmentiBellhopRequest env_req;
    env_req.set_environment_id(env_cfg.environment_id());
    *env_req.mutable_req() = req;
    interprocess().publish<groups::env_bellhop_req>(env_req);
}

void NetSimManager::write_gps_out(const NavUpdate& nav_update)
{
    auto io_msg = std::make_shared<goby::middleware::protobuf::IOData>();
    io_msg->set_index(nav_update.modem_tcp_port());

    auto now_ptime = goby::time::SystemClock::now<boost::posix_time::ptime>();

    boost::format time_fmt("%02d%02d%02d.%06d");
    time_fmt % now_ptime.time_of_day().hours() % now_ptime.time_of_day().minutes() %
        now_ptime.time_of_day().seconds() %
        (now_ptime.time_of_day().fractional_seconds() * 1000000 /
         boost::posix_time::time_duration::ticks_per_second());

    boost::format date_fmt("%02d%02d%02d");
    date_fmt % now_ptime.date().day() % static_cast<int>(now_ptime.date().month()) % (now_ptime.date().year() - now_ptime.date().year()/100*100);

    boost::format lat_fmt("%02d%02d.%04d");

    {
	double lat = std::abs(nav_update.lat());
	int degrees = std::floor(lat);
	int minutes = std::floor((lat - degrees) * 60);
	int ten_thousandth_minutes = std::floor(((lat - degrees) * 60 - minutes) * 10000);
	
	lat_fmt % degrees % minutes % ten_thousandth_minutes;
    }

    boost::format lon_fmt("%03d%02d.%04d");
    {
	double lon = std::abs(nav_update.lon());
	int degrees = std::floor(lon);
	int minutes = std::floor((lon - degrees) * 60);
	int ten_thousandth_minutes = std::floor(((lon - degrees) * 60 - minutes) * 10000);
	
	lon_fmt % degrees % minutes % ten_thousandth_minutes;
    }

    boost::format sog_fmt("%.1f");
    const double meters_per_second_to_knots = 1.94384;    
    sog_fmt % (nav_update.speed()*meters_per_second_to_knots);

    boost::format cmg_fmt("%.1f");
    cmg_fmt % nav_update.heading();    
    
    goby::util::NMEASentence gprmc;
    gprmc.push_back("$GPRMC");
    gprmc.push_back(time_fmt.str()); // time
    gprmc.push_back("A"); // OK
    gprmc.push_back(lat_fmt.str()); // lat, lat hemi
    gprmc.push_back(nav_update.lat() < 0 ? "S" : "N"); // lat, lat hemi
    gprmc.push_back(lon_fmt.str()); // lon, lon hemi
    gprmc.push_back(nav_update.lon() < 0 ? "W" : "E"); // lon, lon hemi
    gprmc.push_back(sog_fmt.str()); // sog, knots
    gprmc.push_back(cmg_fmt.str()); // cmg, degrees
    gprmc.push_back(date_fmt.str()); // date of fix
    
    gprmc.push_back("000.0"); // magnetic variation
    gprmc.push_back("E");

    io_msg->set_data(gprmc.message_cr_nl());

    interthread().publish<groups::gps_line_out>(io_msg);
}
