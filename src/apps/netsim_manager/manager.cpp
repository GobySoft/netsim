#include "goby/middleware/single-thread-application.h"

#include "messages/groups.h"
#include "messages/config_request.pb.h"
#include "lamss/lib_netsim/tcp_server.h"
#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"
#include "lamss/lib_bellhop/iBellhop_messages.pb.h"
#include "messages/manager_config.pb.h"
#include "messages/env_bellhop_req.pb.h"

using namespace goby::common::logger;
using goby::glog;

class NetSimManager : public goby::SingleThreadApplication<NetSimManagerConfig>
{
public:
    NetSimManager();

private:
    void loop() override;
    void process_configuration();
    void process_request(const NetSimManagerRequest& r, const boost::asio::ip::tcp::endpoint& ep);

    void handle_impulse_request(const ImpulseRequest& req);
    void handle_bellhop_request(const iBellhopRequest& req);

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
    goby::SingleThreadApplication<NetSimManagerConfig>(10*boost::units::si::hertz)
{
    process_configuration();

    server_.read_callback<NetSimManagerRequest>(
	[this](const NetSimManagerRequest& r, const boost::asio::ip::tcp::endpoint& ep)
	{ process_request(r, ep); });

    interprocess().subscribe<groups::impulse_request, ImpulseRequest>(
	[this](const ImpulseRequest& req)
	{ handle_impulse_request(req); });

    interprocess().subscribe<groups::config_request, ConfigRequest>(
	[this](const ConfigRequest& req)
	{
	    if(req.subsystem() == ConfigRequest::MANAGER)
		interprocess().publish<groups::configuration>(cfg());
	});


    interprocess().subscribe<groups::bellhop_request, iBellhopRequest>(
	[this](const iBellhopRequest& req)
	{ handle_bellhop_request(req);} );
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
	    interprocess().publish<groups::env_nav_update>(env_nav_update);
	else
	    glog.is(WARN) && glog << "Uninitialized EnvironmentNavUpdate: " << env_nav_update.DebugString() << std::endl;
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
