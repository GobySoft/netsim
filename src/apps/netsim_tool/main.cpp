
#include "goby/acomms/modemdriver/mm_driver.h"

#include "goby/middleware/multi-thread-application.h"

#include "goby/util/geodesy.h"


#include "config.pb.h"
#include "messages/groups.h"
#include "messages/tool.pb.h"

#include "lamss/lib_netsim/tcp_client.h"
#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"

using namespace goby::common::logger;
using goby::glog;

using goby::acomms::protobuf::ModemTransmission;

std::mutex driver_mutex;


class ModemThread : public goby::SimpleThread<goby::acomms::protobuf::DriverConfig>
{
public:
    ModemThread(const goby::acomms::protobuf::DriverConfig& cfg, int index) :
        goby::SimpleThread<goby::acomms::protobuf::DriverConfig>(cfg, 10*boost::units::si::hertz, index)
        {
            driver_.signal_receive.connect(
                [this](const ModemTransmission& msg)
                { interthread().publish<groups::tool::receive_data>(msg); } );

            driver_.startup(cfg);

            int i = index;
            interthread().publish<groups::tool::ready>(index);

            interthread().subscribe<groups::tool::transmit, ModemTransmission>(
                [this](const ModemTransmission& msg)
                {
                    if(this->index() == msg.src())
                    {
                        std::lock_guard<std::mutex> lock(driver_mutex);
                        driver_.handle_initiate_transmission(msg);
                    }
                    
                });
        }

    ~ModemThread()
        {
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


class NetSimTool : public goby::MultiThreadApplication<NetSimToolConfig>
{
public:
    NetSimTool() : goby::MultiThreadApplication<NetSimToolConfig>(10*boost::units::si::hertz),
        r_(cfg().r_min()),
        z_(cfg().z_min()),
        last_r_(-1),
        last_z_(-1),
        geodesy_({ cfg().lat_origin()*boost::units::degree::degrees,
                    cfg().lon_origin()*boost::units::degree::degrees })
        {
            goby::glog.add_group("data_out", goby::common::Colors::yellow);    

            launch_thread<ModemThread>(cfg().tx_driver_cfg().modem_id(), cfg().tx_driver_cfg());
            launch_thread<ModemThread>(cfg().rx_driver_cfg().modem_id(), cfg().rx_driver_cfg());
        
            interthread().subscribe<groups::tool::receive_data, ModemTransmission>(
                [this](const ModemTransmission& msg)
                {
                    glog.is(VERBOSE) && glog << "Received data: " << msg.ShortDebugString() << std::endl;
                    if(msg.ExtensionSize(micromodem::protobuf::receive_stat) > 0)
                        previous_stats_ = msg.GetExtension(micromodem::protobuf::receive_stat, 0);
                }
                );

            interthread().subscribe<groups::tool::ready, int>(
                [this](const int& driver_id)
                {
                    glog.is(VERBOSE) && glog << "Driver " << driver_id << " is ready" << std::endl;
                    ++drivers_ready_;
                } );

            client_->connect(cfg().netsim_manager_ip_addr(), cfg().netsim_manager_tcp_port());
            client_->read_callback<NetSimManagerResponse>(
                [](const NetSimManagerResponse& t, const boost::asio::ip::tcp::endpoint& ep)
                {
                    glog.is(DEBUG1) && glog << "Received response from [" << ep << "]: " << t.ShortDebugString() << std::endl;
                });

            while(!client_->connected())
            {
                usleep(10000);
                io_.poll();
            }
            
            glog.is(VERBOSE) && glog << "Connected to netsim_manager" << std::endl;
            
            NetSimManagerRequest req;
            req.set_id(request_id_++);
            auto& nav = *req.add_nav();
            nav.set_modem_tcp_port(62000);
            nav.set_time(goby::common::goby_time<double>());

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
            if(drivers_ready_ < 2)
                return;

            // if slot is 10 seconds, update the receiver position after 5 seconds
            if(loops_since_last_transmission_ == cfg().transmission().slot_seconds()*loop_frequency_hertz()/2)
            {
                if(!run_complete_)
                    update_receiver_position();
            }
            else if(loops_since_last_transmission_ == 2*cfg().transmission().slot_seconds()*loop_frequency_hertz())
            {

                if(last_r_ > 0 && last_z_ > 0)
                {
                    // publish previous stats
                    netsim::protobuf::ToolReceiveStats tool_stats;
                    if(previous_stats_.IsInitialized())
                        *tool_stats.mutable_stats() = previous_stats_;
                    tool_stats.set_source_r(0);
                    tool_stats.set_source_z(cfg().source_z());
                    tool_stats.set_receiver_r(last_r_);
                    tool_stats.set_receiver_z(last_z_);

                    glog.is(VERBOSE) && glog << group("data_out") << tool_stats.ShortDebugString() << std::endl;
                
                    interprocess().publish<groups::tool::rx_stats>(tool_stats);

                
                    previous_stats_.Clear();
                }
                

                // after publishing last slot, finish up.
                if(run_complete_)
                {
                    quit();
                    return;
                }
                

                // initiate transmission
                glog.is(VERBOSE) && glog << "Starting transmission at: r=" << r_ << ", z=" << z_ << std::endl;

                interthread().publish<groups::tool::transmit>(cfg().transmission());

                
                last_r_ = r_;
                last_z_ = z_;
                
                r_ += cfg().dr();
                if(r_ > cfg().r_max())
                {
                    r_ = cfg().r_min();
                    z_ += cfg().dz();
                    
                    if(z_ > cfg().z_max())
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
            NetSimManagerRequest req;
            req.set_id(request_id_++);
            auto& nav = *req.add_nav();
            nav.set_modem_tcp_port(62001);
            nav.set_time(goby::common::goby_time<double>());

            auto latlon = geodesy_.convert(goby::util::UTMGeodesy::XYPoint({ r_*boost::units::si::meters, 0*boost::units::si::meters}));
                            
            nav.set_lat(latlon.lat / boost::units::degree::degrees);
            nav.set_lon(latlon.lon / boost::units::degree::degrees);
            nav.set_depth(z_);
            nav.set_speed(0);
            nav.set_heading(0);
            
            if(client_->connected())
            {
                glog.is(DEBUG1) && glog << "Sent request (r=" << r_ << ", z=" << z_ << ": " << req.ShortDebugString() << std::endl;
                client_->write(req);
            }
                
        }
    
private:
    int r_;
    int z_;
    int last_r_;
    int last_z_;
    int loops_since_last_transmission_{0};
    int drivers_ready_ {0};
    bool run_complete_{false};

    boost::asio::io_service io_;
    std::shared_ptr<netsim::tcp_client> client_{netsim::tcp_client::create(io_)};
    int request_id_ {0};

    goby::util::UTMGeodesy geodesy_;

    micromodem::protobuf::ReceiveStatistics previous_stats_;
    
};


int main(int argc, char* argv[])
{ return goby::run<NetSimTool>(argc, argv); }


