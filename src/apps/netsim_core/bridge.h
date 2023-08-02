// Copyright 2017-2020:
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

#ifndef BRIDGE20230802H
#define BRIDGE20230802H

#include "netsim/core/boost_serialization.h"

#include "goby/util/debug_logger.h"
#include "goby/zeromq/application/multi_thread.h"

#include "netsim/core/common.h"
#include "netsim/messages/core_config.pb.h"
#include "netsim/messages/groups.h"

using ThreadBase = goby::middleware::SimpleThread<netsim::protobuf::NetSimCoreConfig>;

extern std::atomic<int> bridge_ready;

class BridgeThread : public ThreadBase
{
  public:
    BridgeThread(const netsim::protobuf::NetSimCoreConfig& config) : ThreadBase(config, 0)
    {
        using goby::glog;
        using namespace goby::util::logger;

        int first_modem_index = cfg().bridge().first_modem_index();
        int local_number_of_modems = cfg().bridge().local_number_of_modems();

        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            auto bridge_out_callback =
                [this, i](std::shared_ptr<const netsim::TaggedAudioBuffer> buffer)
            { this->bridge_out(buffer, i); };

            auto bridge_in_callback =
                [this, i](std::shared_ptr<const netsim::TaggedAudioBuffer> buffer)
            { this->bridge_in(buffer, i); };

            // subscribe interthread to all local Detectors and subscribe interprocess to all remote Bridges
            switch (i)
            {
#define BRIDGE_SUBSCRIBE_BRIDGE(z, n, _)                                              \
    case n:                                                                           \
        if (n >= first_modem_index && n < first_modem_index + local_number_of_modems) \
        {  glog.is(VERBOSE) && glog << "Bridge: Interthread Subscribing to DetectorAudio " << n << std::endl; \
		interthread()						\
		    .template subscribe<netsim::groups::DetectorAudio<n>::group, \
                                    netsim::TaggedAudioBuffer>(bridge_out_callback);} \
        else								\
	{ glog.is(VERBOSE) && glog << "Bridge: Interprocess Subscribing to BridgeAudio " << n << std::endl; \
	    interprocess()						\
                .template subscribe<netsim::groups::BridgeAudio<n>::group, \
                                    netsim::TaggedAudioBuffer,		\
                                    goby::middleware::BOOST_SERIALIZATION_SCHEME>( \
					bridge_in_callback); }		\
	break;

                BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, BRIDGE_SUBSCRIBE_BRIDGE, nil)
            }
        }

        glog.is(VERBOSE) && glog << "Starting bridge thread" << std::endl;
        ++bridge_ready;
    }

  private:
    void bridge_out(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer, int from_index)
    {
        using goby::glog;
        using namespace goby::util::logger;

        // publish to appropriate Bridge group (for remote)
	if(buffer->marker == netsim::TaggedAudioBuffer::Marker::START)
	{
	    glog.is_verbose() && glog << "Bridge: Sending START buffer from " << from_index << ", id: " << buffer->packet_id << std::endl;
	}
	else if( buffer->marker == netsim::TaggedAudioBuffer::Marker::END)
	{	
	    glog.is_verbose() && glog << "Bridge: Sending END buffer from " << from_index << ", id: " << buffer->packet_id << std::endl;
	}
	
	
        switch (from_index)
        {
#define BRIDGE_PUBLISH_BRIDGE_OUT(z, n, _)                                                      \
    case n:                                                                                     \
                                                                                                 \
         interprocess()                                                                          \
             .template publish<netsim::groups::BridgeAudio<n>::group, netsim::TaggedAudioBuffer, \
                               goby::middleware::BOOST_SERIALIZATION_SCHEME>(buffer); \
            break;
            BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, BRIDGE_PUBLISH_BRIDGE_OUT, nil)
        }
    }

    void bridge_in(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer, int from_index)
    {
        using goby::glog;
        using namespace goby::util::logger;
        // publish to appropriate Detector group (for local)

	if(buffer->marker == netsim::TaggedAudioBuffer::Marker::START)
	{
	    glog.is_verbose() && glog << "Bridge: Received START buffer from " << from_index << ", id: " << buffer->packet_id << std::endl;
	}
	else if( buffer->marker == netsim::TaggedAudioBuffer::Marker::END)
	{	
	    glog.is_verbose() && glog << "Bridge: Received END buffer from " << from_index << ", id: " << buffer->packet_id << std::endl;
	}


	switch (from_index)
        {
#define BRIDGE_PUBLISH_DETECTOR_IN(z, n, _) \
    case n: interthread().template publish<netsim::groups::DetectorAudio<n>::group>(buffer); break;
            BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, BRIDGE_PUBLISH_DETECTOR_IN, nil)
        }
    }

  private:
};

#endif
