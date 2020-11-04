// Copyright 2017-2020:
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

#ifndef COMMON20170816H
#define COMMON20170816H

#include <jack/jack.h>

#include <vector>

#include <boost/preprocessor/repetition/repeat.hpp>

// clang-format off
#define NETSIM_MAX_MODEMS @NETSIM_MAX_MODEMS@
// clang-format on

namespace netsim
{
typedef float sample_t;

struct AudioBuffer
{
    AudioBuffer(size_t size) : samples(size, 0) {}

    template <typename It> AudioBuffer(It begin, It end) : samples(begin, end) {}

    double buffer_start_time{0};
    jack_nframes_t jack_frame_time{0};

    std::vector<netsim::sample_t> samples;
};

struct TaggedAudioBuffer
{
    std::shared_ptr<const AudioBuffer> buffer;

    enum class Marker
    {
        NONE,
        START,
        END,
        MIDDLE
    };
    Marker marker{Marker::NONE};
    int packet_id{-1};
};

} // namespace netsim

#endif
