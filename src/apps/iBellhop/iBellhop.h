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

#ifndef iBellhopH
#define iBellhopH

#include <fstream>
#include <map>

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2
#include "goby/moos/goby_moos_app.h"
#else
#include "goby/moos/libmoos_util/tes_moos_app.h"
#endif

#include "goby/moos/moos_protobuf_helpers.h"
#include "netsim/acousticstoolbox/environment.h"
#include "netsim/acousticstoolbox/iBellhop_messages.pb.h"
#include "netsim/acousticstoolbox/svp_request_response.pb.h"
#include <boost/date_time.hpp>

class CiBellhop :    public goby::moos::GobyMOOSApp
{
  public:
    static CiBellhop* get_instance();

  private:
    CiBellhop();
    virtual ~CiBellhop();

    struct NodeReport
    {
        double time;
        double x;
        double y;
        double depth;
        double speed;
        double heading;
    };

    static const double pi;

  private:
    void loop() {}

    void handle_ssp_update(const CMOOSMsg& msg);
    void handle_ssp_request(const CMOOSMsg& msg);

    // finds an existing sample for a given depth; failing that, add a new sample for this depth
    netsim::bellhop::protobuf::Environment::WaterColumn::SSPSample*
    find_sample(netsim::bellhop::protobuf::Environment* env, double depth);
    netsim::bellhop::protobuf::Environment::WaterColumn::SSPSample*
    get_sample(netsim::bellhop::protobuf::Environment* env, int i);

    // Interpolates ssp to get sound speed at specific depth
    double get_ssp_value(netsim::bellhop::protobuf::Environment* env, double depth);

    // extracts current environment updated with eofs
    netsim::bellhop::protobuf::Environment current_env();

    void handle_node_report(const CMOOSMsg& msg);
    void handle_nav_x(const CMOOSMsg& msg)
    {
        node_reports_[cfg_.initial_env().adaptive_info().ownship()].x = msg.GetDouble();
        node_reports_[cfg_.initial_env().adaptive_info().ownship()].time = msg.GetTime();
    }

    void handle_nav_y(const CMOOSMsg& msg)
    {
        node_reports_[cfg_.initial_env().adaptive_info().ownship()].y = msg.GetDouble();
    }

    void handle_nav_depth(const CMOOSMsg& msg);

    void handle_nav_speed(const CMOOSMsg& msg)
    {
        node_reports_[cfg_.initial_env().adaptive_info().ownship()].speed = msg.GetDouble();
    }

    void handle_nav_heading(const CMOOSMsg& msg)
    {
        node_reports_[cfg_.initial_env().adaptive_info().ownship()].heading = msg.GetDouble();
    }

    void handle_request(const CMOOSMsg& msg);

    bool update_positions(netsim::bellhop::protobuf::Environment& env);
    void read_shd(netsim::bellhop::protobuf::Environment& env,
                  netsim::protobuf::iBellhopResponse* response, bool full_shd_matrix);

    void calculate_env(netsim::bellhop::protobuf::Environment& env, const std::string& requestor,
                       std::string* output_file);

    friend std::ostream& operator<<(std::ostream& out, const CiBellhop::NodeReport& nr);

  private:
    boost::posix_time::ptime last_calc_time_;
    std::ofstream env_out_;
    std::ofstream ssp_out_;
    std::ofstream bty_out_;
    std::ofstream trc_out_;
    std::ofstream brc_out_;
    std::map<std::string, NodeReport> node_reports_;

    static netsim::protobuf::iBellhopConfig cfg_;
    static CiBellhop* inst_;
};

inline std::ostream& operator<<(std::ostream& out, const CiBellhop::NodeReport& nr)
{
    return (out << "X=" << nr.x << ",Y=" << nr.y << ",DEPTH=" << nr.depth << ",SPD=" << nr.speed
                << ",HDG=" << nr.heading);
}

namespace netsim
{
namespace bellhop
{
namespace protobuf
{
inline bool operator<(const Environment::WaterColumn::SSPSample& a,
                      const Environment::WaterColumn::SSPSample& b)
{
    return a.depth() < b.depth();
}
} // namespace protobuf
} // namespace bellhop
} // namespace netsim

#endif
