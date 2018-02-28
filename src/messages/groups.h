#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace groups
{
    // published by modemsim
    constexpr goby::Group impulse_request{"impulse_request"};
    constexpr goby::Group buffer_size_change{"buffer_size_change"};

    // published by netsim_manager
    constexpr goby::Group env_nav_update{"environment_nav_update"};
    constexpr goby::Group env_impulse_req{"environment_impulse_request"};

    // published by goby_moos_gateway
    constexpr goby::Group impulse_response{"impulse_response"};
    
}

#endif
