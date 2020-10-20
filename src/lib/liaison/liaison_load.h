// Copyright 2018-2019:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
