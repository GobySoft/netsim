// Copyright 2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the NETSIM Binaries.
//
// The NETSIM Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The NETSIM Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#include "processor.h"

void netsim_launch_processor_thread(
    goby::zeromq::MultiThreadApplication<netsim::protobuf::NetSimCoreConfig>* handler,
    int output_index)
{
    switch (output_index)
    {
#define LAUNCH_PROCESSOR(z, n, _) \
    case n: handler->launch_thread<ProcessorThread<n>>(); break;
        BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, LAUNCH_PROCESSOR, nil)
    }
}
