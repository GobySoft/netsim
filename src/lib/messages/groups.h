// Copyright 2017-2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Henrik Schmidt <henrik@mit.edu>
//
//
// This file is part of the NETSIM Libraries.
//
// The NETSIM Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The NETSIM Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

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

template <int from_index> struct AudioIn
{
    static_assert(from_index < 16, "Max supported modems is 16");
    constexpr static goby::middleware::Group group{"ain", from_index};
};
template <int from_index> constexpr goby::middleware::Group AudioIn<from_index>::group;

template <int from_index, int to_index> struct AudioOut
{
    static_assert(from_index < 16 && to_index < 16, "Max supported modems is 16");
    constexpr static goby::middleware::Group group{"aout", from_index*16 + to_index};
};
template <int from_index, int to_index>
constexpr goby::middleware::Group AudioOut<from_index, to_index>::group;


template <int from_index> struct DetectorAudio
{
    static_assert(from_index < 16, "Max supported modems is 16");
    constexpr static goby::middleware::Group group{"detector_audio_from", from_index};
};
template <int from_index> constexpr goby::middleware::Group DetectorAudio<from_index>::group;

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
