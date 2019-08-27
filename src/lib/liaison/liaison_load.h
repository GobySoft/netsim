#ifndef LIAISONLOAD20180820H
#define LIAISONLOAD20180820H

#include <vector>

#include "goby/zeromq/liaison/liaison_container.h"
#include "goby/zeromq/application/multi_thread.h"

extern "C"
{
    std::vector<goby::zeromq::LiaisonContainer*> goby3_liaison_load(
        const goby::apps::zeromq::protobuf::LiaisonConfig& cfg);    
}

    
#endif
