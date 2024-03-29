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

#ifndef DETECTOR20170816H
#define DETECTOR20170816H

#include <boost/circular_buffer.hpp>

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "jack_thread.h"
#include "netsim/messages/core_config.pb.h"

using ThreadBase = goby::middleware::SimpleThread<netsim::protobuf::NetSimCoreConfig>;

extern std::atomic<int> detector_ready;

template <int from_index> class DetectorThread : public ThreadBase
{
  public:
    DetectorThread(const netsim::protobuf::NetSimCoreConfig& config) : ThreadBase(config, 0)
    {
        using goby::glog;
        using namespace goby::util::logger;

        auto audio_in_callback = [this](std::shared_ptr<const netsim::AudioBuffer> buffer) {
            this->audio_in(buffer);
        };

        interthread()
            .template subscribe<netsim::groups::AudioIn<from_index>::group, netsim::AudioBuffer>(
                audio_in_callback);
        interthread().template subscribe<netsim::groups::buffer_size_change, jack_nframes_t>(
            [this](const jack_nframes_t& buffer_size) {
                size_t new_prebuffer_size =
                    (cfg().sampling_freq() * cfg().detector().packet_begin_prebuffer_seconds()) /
                    buffer_size;
                glog.is(VERBOSE) && glog << "Resizing prebuffer to: " << new_prebuffer_size
                                         << std::endl;
                prebuffer_.resize(new_prebuffer_size > 0 ? new_prebuffer_size : 1);
            });

        glog.is(VERBOSE) && glog << "Starting detector thread: " << from_index << std::endl;
        ++detector_ready;
    }

    void audio_in(std::shared_ptr<const netsim::AudioBuffer> buffer)
    {
        using goby::glog;
        using namespace goby::util::logger;

        glog.is(DEBUG2) && glog << "Detector Thread (" << from_index
                                << "): Received buffer of size: " << buffer->samples.size()
                                << std::endl;

        static std::atomic<int> global_packet_id{0};

        if (!in_packet_)
        {
            prebuffer_.push_back(buffer);
            for (auto it = buffer->samples.begin(), end = buffer->samples.end(); it != end; ++it)
            {
                if (std::abs(*it) >= cfg().detector().detection_threshold())
                {
                    // send subbuffer from start of packet to end of buffer
                    in_packet_ = true;
                    in_packet_id = global_packet_id++;
		    in_packet_start_time_ = buffer->buffer_start_time;
		    
                    // assume a packet spans at least one buffer, so no need to check the rest of the buffer
                    glog.is(DEBUG1) && glog
                                           << "Detector Thread (" << from_index
                                           << "): Detected START (id: " << in_packet_id << ", time: " << std::setprecision(15)
                                           << buffer->buffer_start_time << ")" << std::endl;
		    
                    if (cfg().continuous())
                    {
                        std::shared_ptr<netsim::TaggedAudioBuffer> tagged_buffer(
                            new netsim::TaggedAudioBuffer);
                        tagged_buffer->buffer = buffer;
                        tagged_buffer->marker = netsim::TaggedAudioBuffer::Marker::START;
                        tagged_buffer->packet_id = in_packet_id;
                        interthread()
                            .template publish<netsim::groups::DetectorAudio<from_index>::group>(
                                tagged_buffer);
                    }
                    else
                    {
                        auto& first_buffer = prebuffer_.front();

                        if (!first_buffer)
                            break;

                        std::shared_ptr<netsim::AudioBuffer> subbuffer(
                            new netsim::AudioBuffer(*first_buffer));

                        std::shared_ptr<netsim::TaggedAudioBuffer> tagged_subbuffer(
                            new netsim::TaggedAudioBuffer);
                        tagged_subbuffer->buffer = subbuffer;

                        tagged_subbuffer->marker = netsim::TaggedAudioBuffer::Marker::START;
                        tagged_subbuffer->packet_id = in_packet_id;

                        interthread()
                            .template publish<netsim::groups::DetectorAudio<from_index>::group>(
                                tagged_subbuffer);
                        prebuffer_.pop_front();

                        while (!prebuffer_.empty())
                        {
                            std::shared_ptr<netsim::TaggedAudioBuffer> tagged_buffer(
                                new netsim::TaggedAudioBuffer);
                            tagged_buffer->buffer = prebuffer_.front();
                            tagged_buffer->packet_id = in_packet_id;
                            interthread()
                                .template publish<netsim::groups::DetectorAudio<from_index>::group>(
                                    tagged_buffer);
                            prebuffer_.pop_front();
                        }
                    }

                    break;
                }
            }

            if (cfg().continuous() && !in_packet_)
            {
                // send whole buffer
                std::shared_ptr<netsim::TaggedAudioBuffer> tagged_buffer(
                    new netsim::TaggedAudioBuffer);
                tagged_buffer->buffer = buffer;
                tagged_buffer->packet_id = in_packet_id;
                interthread().template publish<netsim::groups::DetectorAudio<from_index>::group>(
                    tagged_buffer);
            }
        }
        else
        {
            for (auto it = buffer->samples.begin(), end = buffer->samples.end(); it != end; ++it)
            {
                if (std::abs(*it) >= cfg().detector().detection_threshold())
                    frames_since_silence = 0;
                else
                    ++frames_since_silence;

                if (frames_since_silence >=
                    cfg().detector().packet_end_silent_seconds() * cfg().sampling_freq())
                {
                    // send subbuffer from beginning of the buffer to the
                    // end of the silent period following packet
                    in_packet_ = false;
                    frames_since_silence = 0;
                    std::shared_ptr<netsim::AudioBuffer> subbuffer(
                        new netsim::AudioBuffer(*buffer));
                    subbuffer->buffer_start_time = buffer->buffer_start_time;

                    std::shared_ptr<netsim::TaggedAudioBuffer> tagged_subbuffer(
                        new netsim::TaggedAudioBuffer);
                    tagged_subbuffer->buffer = subbuffer;
                    tagged_subbuffer->marker = netsim::TaggedAudioBuffer::Marker::END;
                    tagged_subbuffer->packet_id = in_packet_id;

                    interthread()
                        .template publish<netsim::groups::DetectorAudio<from_index>::group>(
                            tagged_subbuffer);
                    glog.is(DEBUG1) &&
                        glog << "Detector Thread (" << from_index
			     << "): Detected END (id: " << in_packet_id << ", time: " << std::setprecision(15)
                             << subbuffer->buffer_start_time +
		                static_cast<double>(it - buffer->samples.begin()) /
			static_cast<double>(cfg().sampling_freq()) << ", duration: " << subbuffer->buffer_start_time + static_cast<double>(it - buffer->samples.begin()) /
			static_cast<double>(cfg().sampling_freq()) - in_packet_start_time_ << ")"
                             << std::endl;
                    // assume no new packet starts within the same buffer
                    break;
                }
            }
            if (in_packet_ || cfg().continuous())
            {
                // send whole buffer
                std::shared_ptr<netsim::TaggedAudioBuffer> tagged_buffer(
                    new netsim::TaggedAudioBuffer);
                tagged_buffer->buffer = buffer;
                if (in_packet_)
                    tagged_buffer->marker = netsim::TaggedAudioBuffer::Marker::MIDDLE;
                tagged_buffer->packet_id = in_packet_id;
                interthread().template publish<netsim::groups::DetectorAudio<from_index>::group>(
                    tagged_buffer);
            }
        }
    }

  private:
    bool in_packet_{false};
    int in_packet_id{-1};
    double in_packet_start_time_{0};
    
    jack_nframes_t frames_since_silence{0};

    boost::circular_buffer<std::shared_ptr<const netsim::AudioBuffer>> prebuffer_{1};
};

#endif
