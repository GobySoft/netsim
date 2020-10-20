// Copyright 2018-2020:
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

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "netsim/messages/groups.h"
#include "audio_images.h"
#include "tl_plot.h"

using namespace goby::util::logger;
using goby::glog;

class NetSimPostprocess : public goby::zeromq::MultiThreadApplication<NetSimPostprocessConfig>
{
public:
    NetSimPostprocess()
        {
            launch_thread<AudioImagesThread>(); 
            launch_thread<TLPlotThread>(); 
	}
};



int main(int argc, char* argv[])
{ return goby::run<NetSimPostprocess>(argc, argv); }
