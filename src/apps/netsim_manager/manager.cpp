#include "goby/middleware/single-thread-application.h"

#include "config.pb.h"
#include "messages/groups.h"
#include "lamss/lib_netsim/tcp_server.h"
#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"

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

	// check bounds
	// CONTINUE FROM HERE
	
    }
   
}
