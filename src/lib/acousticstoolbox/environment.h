// Copyright 2020:
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

#ifndef ENVIRONMENTHELPERS20110310H
#define ENVIRONMENTHELPERS20110310H

#include "netsim/acousticstoolbox/environment.pb.h"

#include <boost/serialization/vector.hpp>

namespace netsim
{
namespace bellhop
{
class Environment
{
  public:
    static void output_env(std::ostream* os, std::ostream* ssp, std::ostream* bty,
                           std::ostream* trc, std::ostream* brc, const protobuf::Environment& env);
};

struct TLMatrix
{
    std::vector<float> depths;
    std::vector<float> ranges;
    // tl[depth][range]
    std::vector<std::vector<float>> tl;

    template <class Archive> void serialize(Archive& ar, const unsigned int version)
    {
        ar& depths;
        ar& ranges;
        ar& tl;
    }
};

} // namespace bellhop
} // namespace netsim

#endif
