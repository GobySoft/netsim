#include "liaison_netsim.h"
#include "liaison_load.h"

extern "C"
{
    std::vector<goby::common::LiaisonContainer*> goby3_liaison_load(
        const goby::common::protobuf::LiaisonConfig& cfg)
    {        
        return { new LiaisonNetsim(cfg) };
    }
}
