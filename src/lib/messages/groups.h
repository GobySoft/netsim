#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace netsim
{
namespace groups
{
// published by netsim_core
constexpr goby::middleware::Group impulse_request{"impulse_request"};
constexpr goby::middleware::Group buffer_size_change{"buffer_size_change"};
constexpr goby::middleware::Group logger_event{"logger_event"};

// published by netsim_postprocess
constexpr goby::middleware::Group post_process_event{"post_process_event"};

// published by netsim_manager
constexpr goby::middleware::Group env_nav_update{"environment_nav_update"};
constexpr goby::middleware::Group env_impulse_req{"environment_impulse_request"};
constexpr goby::middleware::Group env_bellhop_req{"environment_bellhop_request"};
constexpr goby::middleware::Group env_performance_req{"environment_performance_request"};
constexpr goby::middleware::Group configuration{"configuration"};
constexpr goby::middleware::Group receive_stats{"receive_stats"};

constexpr goby::middleware::Group gps_line_out{"gps_line_out"};
constexpr goby::middleware::Group gps_line_in{"gps_line_in"};

// published by netsim_liaison
constexpr goby::middleware::Group config_request{"config_request"};
constexpr goby::middleware::Group bellhop_request{"bellhop_request"};

// published by netsim_udp
constexpr goby::middleware::Group performance_request{"performance_request"};

// published by goby_moos_gateway
constexpr goby::middleware::Group impulse_response{"impulse_response"};
constexpr goby::middleware::Group bellhop_response{"bellhop_response"};
constexpr goby::middleware::Group performance_response{"performance_response"};

// published by netsim_tool
namespace tool
{
constexpr goby::middleware::Group receive_data{"tool::receive_data"};
constexpr goby::middleware::Group ready{"tool::ready"};
constexpr goby::middleware::Group transmit{"tool::transmit"};
constexpr goby::middleware::Group rx_stats{"tool::rx_stats"};
constexpr goby::middleware::Group raw_in{"tool::raw_in"};
constexpr goby::middleware::Group raw_out{"tool::raw_out"};

} // namespace tool

} // namespace groups
} // namespace netsim

#endif
