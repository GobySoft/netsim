// copyright 2010 t. schneider tes@mit.edu
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

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
