#include "goby/moos/middleware/moos_plugin_translator.h"
#include "goby/moos/moos_translator.h"

#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"
#include "messages/groups.h"

using goby::glog;
using namespace goby::common::logger;

class ModemSimTranslation : public goby::moos::Translator
{
public:
    using Base = goby::moos::Translator;
    
    ModemSimTranslation(const GobyMOOSGatewayConfig& cfg) :
        Base(cfg)
        {
            Base::goby_comms().subscribe<groups::impulse_request, ImpulseRequest>(
                [this](const ImpulseRequest& i) { this->goby_to_moos(i); }
                );

            Base::add_moos_trigger(imp_resp_var_);


            goby::moos::protobuf::TranslatorEntry imp_resp_entry;
            imp_resp_entry.set_protobuf_name("ImpulseResponse");
            auto& create = *imp_resp_entry.add_create();
            create.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            create.set_moos_var(imp_resp_var_);
            translator_.add_entry(imp_resp_entry);
            
            goby::moos::protobuf::TranslatorEntry imp_req_entry;
            imp_req_entry.set_protobuf_name("ImpulseRequest");
            auto& publish = *imp_req_entry.add_publish();
            publish.set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            publish.set_moos_var(imp_req_var_);
            translator_.add_entry(imp_req_entry);
        }

private:
    
    void moos_to_goby(const CMOOSMsg& moos_msg) override
        {
            if(moos_msg.GetKey() == imp_resp_var_)
            {
                // publish IMPULSE_RESPONSE
                std::map<std::string, CMOOSMsg> moos_msgs = {{ moos_msg.GetKey(), moos_msg }};
                auto imp_resp_pb = translator_.moos_to_protobuf<std::shared_ptr<google::protobuf::Message>>(moos_msgs, "ImpulseResponse");
                Base::goby_comms().publish<groups::impulse_response>(std::dynamic_pointer_cast<ImpulseResponse>(imp_resp_pb));
            }
        }

    void goby_to_moos(const ImpulseRequest& pos)
        {
            std::multimap<std::string, CMOOSMsg> moos_msgs = translator_.protobuf_to_moos(pos);
            for(auto& moos_msg_pair : moos_msgs)
                Base::moos_comms().Post(moos_msg_pair.second);
        }

private:
    goby::moos::MOOSTranslator translator_;
    const std::string imp_resp_var_{"IMPULSE_RESPONSE"};
    const std::string imp_req_var_{"IMPULSE_REQUEST"};
};

extern "C"
{
    void goby3_moos_gateway_load(goby::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    { handler->launch_thread<ModemSimTranslation>(); }
    
    void goby3_moos_gateway_unload(goby::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    { handler->join_thread<ModemSimTranslation>(); }
}

