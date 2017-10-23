#include "goby/moos/middleware/moos_plugin_translator.h"

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

            Base::add_moos_trigger("IMPULSE_RESPONSE");
            
        }

private:
    
    void moos_to_goby(const CMOOSMsg& moos_msg) override
        {   
            // publish IMPULSE_RESPONSE
        }

    void goby_to_moos(const ImpulseRequest& pos)
        {
            // publish IMPULSE_REQUEST
        }
};

extern "C"
{
    void goby3_moos_gateway_load(goby::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    { handler->launch_thread<ModemSimTranslation>(); }
    
    void goby3_moos_gateway_unload(goby::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    { handler->join_thread<ModemSimTranslation>(); }
}

