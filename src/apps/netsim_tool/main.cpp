// Copyright 2018-2020:
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

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/acomms/modemdriver/mm_driver.h"
#include "goby/time/legacy.h"
#include "goby/util/geodesy.h"
#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "netsim/messages/groups.h"
#include "netsim/messages/tool.pb.h"

#include "netsim/messages/netsim.pb.h"
#include "netsim/tcp/tcp_client.h"

using namespace goby::util::logger;
using goby::glog;
using goby::acomms::protobuf::ModemTransmission;
namespace micromodem = goby::acomms::micromodem;

std::mutex driver_mutex;

class ModemThread : public goby::middleware::SimpleThread<goby::acomms::protobuf::DriverConfig>
{
  public:
    ModemThread(const goby::acomms::protobuf::DriverConfig& cfg, int index)
        : goby::middleware::SimpleThread<goby::acomms::protobuf::DriverConfig>(
              cfg, 10 * boost::units::si::hertz, index)
    {
        {
            std::lock_guard<std::mutex> lock(driver_mutex);
            driver_.signal_receive.connect([this](const ModemTransmission& msg) {
                interprocess().publish<netsim::groups::tool::receive_data>(msg);
            });

            driver_.signal_raw_incoming.connect(
                [this, index](const goby::acomms::protobuf::ModemRaw& raw) {
                    netsim::protobuf::ToolModemRaw tool_raw;
                    tool_raw.set_modem_id(index);
                    *tool_raw.mutable_msg() = raw;
                    interprocess().publish<netsim::groups::tool::raw_in>(tool_raw);
                });
            
            driver_.signal_raw_outgoing.connect(
                [this, index](const goby::acomms::protobuf::ModemRaw& raw) {
                    netsim::protobuf::ToolModemRaw tool_raw;
                    tool_raw.set_modem_id(index);
                    *tool_raw.mutable_msg() = raw;
                    interprocess().publish<netsim::groups::tool::raw_out>(tool_raw);
                });
        }

        {
            std::lock_guard<std::mutex> lock(driver_mutex);
            driver_.startup(cfg);
        }

        int i = index;
        interthread().publish<netsim::groups::tool::ready>(index);

        interthread().subscribe<netsim::groups::tool::transmit, ModemTransmission>(
            [this](const ModemTransmission& msg) {
                if (this->index() == msg.src())
                {
                    std::lock_guard<std::mutex> lock(driver_mutex);
                    driver_.handle_initiate_transmission(msg);
                }
            });
    }

    ~ModemThread()
    {
        std::lock_guard<std::mutex> lock(driver_mutex);
        driver_.shutdown();
    }

  private:
    void loop() override
    {
        std::lock_guard<std::mutex> lock(driver_mutex);
        driver_.do_work();
    }

  private:
    goby::acomms::MMDriver driver_;
};

class NetSimTool : public goby::zeromq::MultiThreadApplication<NetSimToolConfig>
{
  public:
    NetSimTool()
        : goby::zeromq::MultiThreadApplication<NetSimToolConfig>(10 * boost::units::si::hertz),
          r_(cfg().r_min()),
          z_(cfg().z_min()),
          last_r_(-1),
          last_z_(-1),
          geodesy_({cfg().lat_origin() * boost::units::degree::degrees,
                    cfg().lon_origin() * boost::units::degree::degrees})
    {
        goby::glog.add_group("data_out", goby::util::Colors::yellow);

        launch_thread<ModemThread>(cfg().tx_driver_cfg().modem_id(), cfg().tx_driver_cfg());
        launch_thread<ModemThread>(cfg().rx_driver_cfg().modem_id(), cfg().rx_driver_cfg());

        interthread().subscribe<netsim::groups::tool::receive_data, ModemTransmission>(
            [this](const ModemTransmission& msg) {
                glog.is(VERBOSE) && glog << "Received data: " << msg.ShortDebugString()
                                         << std::endl;
                if (msg.HasExtension(micromodem::protobuf::transmission) &&
                    msg.GetExtension(micromodem::protobuf::transmission).receive_stat_size() > 0)
                {
                    previous_stats_ =
                        msg.GetExtension(micromodem::protobuf::transmission).receive_stat(0);

                    netsim::protobuf::NetSimManagerRequest req;
                    req.set_id(request_id_++);
                    auto& stats = *req.add_stats();
                    stats.set_modem_tcp_port(cfg().rx_driver_cfg().tcp_port());
                    stats.set_tx_modem_tcp_port(cfg().tx_driver_cfg().tcp_port());
                    stats.set_packet_success(previous_stats_.mode() ==
                                             micromodem::protobuf::RECEIVE_GOOD);
                    *stats.mutable_mm_stats() = previous_stats_;

                    if (client_->connected())
                    {
                        glog.is(DEBUG1) && glog << "Sent stats: " << req.ShortDebugString()
                                                << std::endl;
                        client_->write(req);
                    }
                }
            });

        interthread().subscribe<netsim::groups::tool::ready, int>([this](const int& driver_id) {
            glog.is(VERBOSE) && glog << "Driver " << driver_id << " is ready" << std::endl;
            ++drivers_ready_;
        });

        client_->connect(cfg().netsim_manager_ip_addr(), cfg().netsim_manager_tcp_port());
        client_->read_callback<netsim::protobuf::NetSimManagerResponse>(
            [](const netsim::protobuf::NetSimManagerResponse& t, const boost::asio::ip::tcp::endpoint& ep) {
                glog.is(DEBUG1) && glog << "Received response from [" << ep
                                        << "]: " << t.ShortDebugString() << std::endl;
            });

        while (!client_->connected())
        {
            usleep(10000);
            io_.poll();
        }

        glog.is(VERBOSE) && glog << "Connected to netsim_manager" << std::endl;

        netsim::protobuf::NetSimManagerRequest req;
        req.set_id(request_id_++);
        auto& nav = *req.add_nav();
        nav.set_modem_tcp_port(cfg().tx_driver_cfg().tcp_port());
        nav.set_time(goby::time::SystemClock::now<goby::time::SITime>().value());

        nav.set_lat(cfg().lat_origin());
        nav.set_lon(cfg().lon_origin());
        nav.set_depth(cfg().source_z());
        nav.set_speed(0);
        nav.set_heading(0);
        glog.is(DEBUG1) && glog << "Sent request: " << req.ShortDebugString() << std::endl;
        client_->write(req);

        update_receiver_position();
    }

  private:
    void loop() override
    {
        if (drivers_ready_ < 2)
            return;

        // if slot is 10 seconds, update the receiver position after 5 seconds
        if (loops_since_last_transmission_ ==
            cfg().transmission().slot_seconds() * loop_frequency_hertz() / 2)
        {
            if (!run_complete_)
                update_receiver_position();
        }
        else if (loops_since_last_transmission_ ==
                 2 * cfg().transmission().slot_seconds() * loop_frequency_hertz())
        {
            if (last_r_ > 0 && last_z_ > 0)
            {
                // publish previous stats
                netsim::protobuf::ToolReceiveStats tool_stats;
                if (previous_stats_.IsInitialized())
                    *tool_stats.mutable_stats() = previous_stats_;
                tool_stats.set_source_r(0);
                tool_stats.set_source_z(cfg().source_z());
                tool_stats.set_receiver_r(last_r_);
                tool_stats.set_receiver_z(last_z_);

                glog.is(VERBOSE) && glog << group("data_out") << tool_stats.ShortDebugString()
                                         << std::endl;

                interprocess().publish<netsim::groups::tool::rx_stats>(tool_stats);

                previous_stats_.Clear();
            }

            // after publishing last slot, finish up.
            if (run_complete_)
            {
                quit();
                return;
            }

            // initiate transmission
            glog.is(VERBOSE) && glog << "Starting transmission at: r=" << r_ << ", z=" << z_
                                     << std::endl;

            interprocess().publish<netsim::groups::tool::transmit>(cfg().transmission());

            last_r_ = r_;
            last_z_ = z_;

            r_ += cfg().dr();
            if (r_ > cfg().r_max())
            {
                r_ = cfg().r_min();
                z_ += cfg().dz();

                if (z_ > cfg().z_max())
                {
                    glog.is(VERBOSE) && glog << "Run complete." << std::endl;
                    run_complete_ = true;
                }
            }
            loops_since_last_transmission_ = 0;
        }
        ++loops_since_last_transmission_;

        io_.poll();
    }

    void update_receiver_position()
    {
        netsim::protobuf::NetSimManagerRequest req;
        req.set_id(request_id_++);
        auto& nav = *req.add_nav();
        nav.set_modem_tcp_port(cfg().rx_driver_cfg().tcp_port());
        nav.set_time(goby::time::SystemClock::now<goby::time::SITime>().value());

        auto latlon = geodesy_.convert(goby::util::UTMGeodesy::XYPoint(
            {r_ * boost::units::si::meters, 0 * boost::units::si::meters}));

        nav.set_lat(latlon.lat / boost::units::degree::degrees);
        nav.set_lon(latlon.lon / boost::units::degree::degrees);
        nav.set_depth(z_);
        nav.set_speed(0);
        nav.set_heading(0);

        if (client_->connected())
        {
            glog.is(DEBUG1) && glog << "Sent request (r=" << r_ << ", z=" << z_ << ": "
                                    << req.ShortDebugString() << std::endl;
            client_->write(req);
        }
    }

  private:
    int r_;
    int z_;
    int last_r_;
    int last_z_;
    int loops_since_last_transmission_{0};
    int drivers_ready_{0};
    bool run_complete_{false};

    boost::asio::io_service io_;
    std::shared_ptr<netsim::tcp_client> client_{netsim::tcp_client::create(io_)};
    int request_id_{0};

    goby::util::UTMGeodesy geodesy_;

    micromodem::protobuf::ReceiveStatistics previous_stats_;
};

int main(int argc, char* argv[]) { return goby::run<NetSimTool>(argc, argv); }
