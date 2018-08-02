#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace groups
{
    // published by netsim
    constexpr goby::Group impulse_request{"impulse_request"};
    constexpr goby::Group buffer_size_change{"buffer_size_change"};

    // published by netsim_manager
    constexpr goby::Group env_nav_update{"environment_nav_update"};
    constexpr goby::Group env_impulse_req{"environment_impulse_request"};

    // published by goby_moos_gateway
    constexpr goby::Group impulse_response{"impulse_response"};

    // published by netsim_tool
    namespace tool
    {
        constexpr goby::Group receive_data{"tool::receive_data"};
        constexpr goby::Group ready{"tool::ready"};
        constexpr goby::Group transmit{"tool::transmit"};
        constexpr goby::Group rx_stats{"tool::rx_stats"};
            
    }
        
}

#endif
