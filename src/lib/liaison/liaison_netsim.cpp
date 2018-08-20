#include <Wt/WFileResource>

#include <Wt/WPanel>

#include <boost/filesystem.hpp>

#include "liaison_netsim.h"

using goby::glog;
using namespace Wt;

LiaisonNetsim::LiaisonNetsim(const goby::common::protobuf::LiaisonConfig& cfg)
    : goby::common::LiaisonContainerWithComms<LiaisonNetsim, NetsimCommsThread>(cfg),
    netsim_cfg_(cfg.GetExtension(protobuf::netsim_config))
{

    set_name("Netsim");
}
