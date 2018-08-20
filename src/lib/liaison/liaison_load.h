#ifndef LIAISONLOAD20180820H
#define LIAISONLOAD20180820H

#include <vector>

#include "goby/middleware/liaison/liaison_container.h"
#include "goby/middleware/multi-thread-application.h"

extern "C"
{
    std::vector<goby::common::LiaisonContainer*> goby3_liaison_load(
        const goby::common::protobuf::LiaisonConfig& cfg);    
}

    
#endif
