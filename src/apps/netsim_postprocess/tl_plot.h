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

#ifndef TLPLOT20180821H
#define TLPLOT20180821H

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "netsim/acousticstoolbox/iBellhop_messages.pb.h"

using ThreadBase = goby::middleware::SimpleThread<NetSimPostprocessConfig>;

class TLPlotThread : public ThreadBase
{
public:

TLPlotThread(const NetSimPostprocessConfig& config)
    : ThreadBase(config)
    {
	using goby::glog;

        interprocess().subscribe<netsim::groups::bellhop_response,
            netsim::protobuf::iBellhopResponse>(
                [this](const netsim::protobuf::iBellhopResponse& resp)
                {
		    if(resp.requestor().find("goby::moos::Translator") != std::string::npos)
		    {
			if(resp.success())
			{
			    
			    std::stringstream octave_cmd;
			    octave_cmd << "octave /opt/netsim/src/octave/liaison_create_shd.m " << resp.output_file();
			    glog.is_debug1() && glog << "Running: " << octave_cmd.str() << std::endl;
			    system(octave_cmd.str().c_str());
			    glog.is_debug1() && glog << "Octave complete" << std::endl;
			}
			
			interprocess().publish<netsim::groups::post_process_event>(resp);
		    }
                });
    }

private:

    
};

#endif
