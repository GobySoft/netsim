#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace groups
{
    // published by netsim_core
    constexpr goby::Group impulse_request{"impulse_request"};
    constexpr goby::Group buffer_size_change{"buffer_size_change"};
    constexpr goby::Group logger_event{"logger_event"};

    // published by netsim_postprocess
    constexpr goby::Group post_process_event{"post_process_event"};
    
    // published by netsim_manager
    constexpr goby::Group env_nav_update{"environment_nav_update"};
    constexpr goby::Group env_impulse_req{"environment_impulse_request"};
    constexpr goby::Group env_bellhop_req{"environment_bellhop_request"};
    constexpr goby::Group configuration{"configuration"};    
    constexpr goby::Group receive_stats{"receive_stats"};
    
    // published by netsim_liaison
    constexpr goby::Group config_request{"config_request"};
    constexpr goby::Group bellhop_request{"bellhop_request"};
    
    // published by goby_moos_gateway
    constexpr goby::Group impulse_response{"impulse_response"};
    constexpr goby::Group bellhop_response{"bellhop_response"};

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
