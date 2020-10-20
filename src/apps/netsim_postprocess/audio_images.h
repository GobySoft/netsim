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

#ifndef AUDIOIMAGES20180820H
#define AUDIOIMAGES20180820H

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "netsim/messages/logger.pb.h"

using ThreadBase = goby::middleware::SimpleThread<NetSimPostprocessConfig>;

class AudioImagesThread : public ThreadBase
{
public:

AudioImagesThread(const NetSimPostprocessConfig& config)
    : ThreadBase(config)
    {
	using goby::glog;

        interprocess().subscribe<netsim::groups::logger_event,
            netsim::protobuf::LoggerEvent>(
                [this](const netsim::protobuf::LoggerEvent& event)
                {
		    if(event.event() == netsim::protobuf::LoggerEvent::ALL_LOGS_CLOSED_FOR_PACKET)
		    {
                        std::stringstream octave_cmd, convert_cmd;
			octave_cmd << "flatpak run org.octave.Octave /opt/netsim/src/octave/liaison_plot_signal.m " << event.log_dir() << " " << event.start_time() << " " << event.packet_id();
			glog.is_debug1() && glog << "Running: " << octave_cmd.str() << std::endl;
			system(octave_cmd.str().c_str());

			convert_cmd << "/opt/netsim/scripts/convert_eps.sh " << event.log_dir();
			glog.is_debug1() && glog << "Running: " << convert_cmd.str() << std::endl;
			system(convert_cmd.str().c_str());
			glog.is_debug1() && glog << "Octave / ImageMagick complete" << std::endl;
		    }
                    interprocess().publish<netsim::groups::post_process_event>(event);
                });
    }

private:

    
};

#endif
