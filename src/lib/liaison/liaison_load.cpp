#include "liaison_netsim.h"
#include "liaison_load.h"

extern "C"
{
    std::vector<goby::zeromq::LiaisonContainer*> goby3_liaison_load(
        const goby::apps::zeromq::protobuf::LiaisonConfig& cfg)
    {        
        return { new netsim::LiaisonNetsim(cfg) };
    }
}
