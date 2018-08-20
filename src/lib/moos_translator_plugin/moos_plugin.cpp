#include "goby/moos/middleware/moos_plugin_translator.h"
#include "goby/moos/moos_translator.h"

#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"
#include "messages/groups.h"
#include "messages/env_bellhop_req.pb.h"

using goby::glog;
using namespace goby::common::logger;

class NetSimCoreTranslation : public goby::moos::Translator
{
public:
    using Base = goby::moos::Translator;
    
    NetSimCoreTranslation(const GobyMOOSGatewayConfig& cfg) :
        Base(cfg),
	environment_id_(cfg.moos_port() % 10) // based on MOOS port 0-9
        {
	    glog.is(DEBUG1) && glog << "Environment id: " << environment_id_ << std::endl;

            Base::goby_comms().interprocess().subscribe<groups::env_impulse_req, EnvironmentImpulseRequest>(
                [this](const EnvironmentImpulseRequest& i) { this->goby_to_moos(i); }
                );

            Base::goby_comms().interprocess().subscribe<groups::env_nav_update, EnvironmentNavUpdate>(
                [this](const EnvironmentNavUpdate& n) { this->goby_to_moos(n); }
                );

            Base::goby_comms().interprocess().subscribe<groups::env_bellhop_req, EnvironmentiBellhopRequest>(
                [this](const EnvironmentiBellhopRequest& n) { this->goby_to_moos(n); }
                );

	    
	    
            Base::add_moos_trigger(imp_resp_var_);
            Base::add_moos_trigger(bellhop_resp_var_);
	    

	    {
		goby::moos::protobuf::TranslatorEntry imp_resp_entry;
		imp_resp_entry.set_protobuf_name(ImpulseResponse::descriptor()->full_name());
		auto& create = *imp_resp_entry.add_create();
		create.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
		create.set_moos_var(imp_resp_var_);
		translator_.add_entry(imp_resp_entry);
	    }

	    {
		goby::moos::protobuf::TranslatorEntry imp_req_entry;
		imp_req_entry.set_protobuf_name(ImpulseRequest::descriptor()->full_name());
		auto& publish = *imp_req_entry.add_publish();
		publish.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
		publish.set_moos_var(imp_req_var_);
		translator_.add_entry(imp_req_entry);
	    }

	    {
		goby::moos::protobuf::TranslatorEntry nav_update_entry;
		nav_update_entry.set_protobuf_name(NavUpdate::descriptor()->full_name());
		auto& publish = *nav_update_entry.add_publish();
		publish.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
		publish.set_moos_var(nav_update_var_);
		translator_.add_entry(nav_update_entry);
	    }

	    {
		goby::moos::protobuf::TranslatorEntry bellhop_resp_entry;
		bellhop_resp_entry.set_protobuf_name(iBellhopResponse::descriptor()->full_name());
		auto& create = *bellhop_resp_entry.add_create();
		create.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
		create.set_moos_var(bellhop_resp_var_);
		translator_.add_entry(bellhop_resp_entry);
	    }

	    {
		goby::moos::protobuf::TranslatorEntry bellhop_req_entry;
		bellhop_req_entry.set_protobuf_name(iBellhopRequest::descriptor()->full_name());
		auto& publish = *bellhop_req_entry.add_publish();
		publish.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
		publish.set_moos_var(bellhop_req_var_);
		translator_.add_entry(bellhop_req_entry);
	    }

	    
	}

private:
    
    void moos_to_goby(const CMOOSMsg& moos_msg) override
        {
            if(moos_msg.GetKey() == imp_resp_var_)
            {
                // publish IMPULSE_RESPONSE
                std::map<std::string, CMOOSMsg> moos_msgs = {{ moos_msg.GetKey(), moos_msg }};
                auto imp_resp_pb = translator_.moos_to_protobuf<std::shared_ptr<google::protobuf::Message>>(moos_msgs, "ImpulseResponse");
                Base::goby_comms().interprocess().publish<groups::impulse_response>(std::dynamic_pointer_cast<ImpulseResponse>(imp_resp_pb));
            }
        }

    void goby_to_moos(const EnvironmentImpulseRequest& req)
        {
	    if(req.environment_id() == environment_id_)
	    {	       
		std::multimap<std::string, CMOOSMsg> moos_msgs = translator_.protobuf_to_moos(req.req());
		for(auto& moos_msg_pair : moos_msgs)
		    Base::moos_comms().Post(moos_msg_pair.second);
	    }
        }

    void goby_to_moos(const EnvironmentNavUpdate& update)
        {
	    if(update.environment_id() == environment_id_)
	    {
		std::multimap<std::string, CMOOSMsg> moos_msgs = translator_.protobuf_to_moos(update.nav());
		for(auto& moos_msg_pair : moos_msgs)
		    Base::moos_comms().Post(moos_msg_pair.second);
	    }
        }

    void goby_to_moos(const EnvironmentiBellhopRequest& req)
        {
	    if(req.environment_id() == environment_id_)
	    {
		std::multimap<std::string, CMOOSMsg> moos_msgs = translator_.protobuf_to_moos(req.req());
		for(auto& moos_msg_pair : moos_msgs)
		    Base::moos_comms().Post(moos_msg_pair.second);
	    }
        }

    
private:
    goby::moos::MOOSTranslator translator_;
    const std::string imp_resp_var_{"IMPULSE_RESPONSE"};
    const std::string imp_req_var_{"IMPULSE_REQUEST"};
    const std::string nav_update_var_{"NETSIM_NAV_UPDATE"};

    const std::string bellhop_req_var_{"BELLHOP_REQUEST"};
    const std::string bellhop_resp_var_{"BELLHOP_RESPONSE"};

    int environment_id_;

};

extern "C"
{
    void goby3_moos_gateway_load(goby::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    { handler->launch_thread<NetSimCoreTranslation>(); }
    
    void goby3_moos_gateway_unload(goby::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    { handler->join_thread<NetSimCoreTranslation>(); }
}

