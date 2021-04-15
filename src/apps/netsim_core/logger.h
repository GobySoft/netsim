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

#ifndef LOGGER20171025H
#define LOGGER20171025H

#include <boost/preprocessor/arithmetic/mul.hpp>
#include <fstream>
#include <sstream>

#include "goby/zeromq/application/multi_thread.h"
#include "netsim/core/common.h"
#include "netsim/messages/core_config.pb.h"
#include "netsim/messages/groups.h"
#include "netsim/messages/logger.pb.h"

using ThreadBase = goby::middleware::SimpleThread<netsim::protobuf::NetSimCoreConfig>;

class LoggerThread : public ThreadBase
{
  private:
    enum class Direction
    {
        IN,
        OUT
    };

  public:
    LoggerThread(const netsim::protobuf::NetSimCoreConfig& config) : ThreadBase(config, 0)
    {
        // subscribe to all the detector output
        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            auto detector_audio_callback =
                [this, i](std::shared_ptr<const netsim::TaggedAudioBuffer> buffer) {
                    this->log_audio(buffer, i, -1, Direction::IN);
                };

            switch (i)
            {
#define NETSIM_LOGGER_THREAD_SUBSCRIBE_DETECTOR_AUDIO(z, n, _)                       \
    case n:                                                                          \
        interthread()                                                                \
            .template subscribe<netsim::groups::DetectorAudio<n>::group,             \
                                netsim::TaggedAudioBuffer>(detector_audio_callback); \
        break;
                BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, NETSIM_LOGGER_THREAD_SUBSCRIBE_DETECTOR_AUDIO,
                                nil)
            }
        }

        // subscribe to all the processor output
        for (int from_i = 0, n = cfg().number_of_modems(); from_i < n; ++from_i)
        {
            for (int to_i = 0, m = cfg().number_of_modems(); to_i < m; ++to_i)
            {
                auto audio_out_callback =
                    [this, from_i, to_i](std::shared_ptr<const netsim::TaggedAudioBuffer> buffer) {
                        this->log_audio(buffer, from_i, to_i, Direction::OUT);
                    };

                switch (from_i * NETSIM_MAX_MODEMS + to_i)
                {
#define NETSIM_LOGGER_THREAD_SUBSCRIBE_AUDIO_OUT(z, n, _)                                      \
    case n:                                                                                    \
        interthread()                                                                          \
            .template subscribe<                                                               \
                netsim::groups::AudioOut<n / NETSIM_MAX_MODEMS, n % NETSIM_MAX_MODEMS>::group, \
                netsim::TaggedAudioBuffer>(audio_out_callback);                                \
        break;
                    BOOST_PP_REPEAT(BOOST_PP_MUL(NETSIM_MAX_MODEMS, NETSIM_MAX_MODEMS),
                                    NETSIM_LOGGER_THREAD_SUBSCRIBE_AUDIO_OUT, nil)
                }
            }
        }
    }

  private:
    void log_audio(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer, int from_modem_index,
                   int to_modem_index, Direction dir)
    {
        using goby::glog;
        using namespace goby::util::logger;

        int modem_index = (dir == Direction::IN) ? from_modem_index : to_modem_index;

        if (buffer->marker == netsim::TaggedAudioBuffer::Marker::START)
        {
            std::stringstream file_name;
	    
            file_name << cfg().logger().log_directory() << "/netsim_" << start_time << "_"
                      << ((dir == Direction::IN) ? "in_" : "out_") << std::setw(3)
                      << std::setfill('0') << buffer->packet_id << "_tx"
                      << std::to_string(from_modem_index);

	    if(dir == Direction::OUT)
		file_name << "_rx" << to_modem_index;

	    file_name << ".bin";

            files_[dir][buffer->packet_id][modem_index].reset(
                new std::ofstream(file_name.str().c_str(), std::ios::out | std::ios::binary));

            // write double time to the start of the file
            auto& file_ptr = files_[dir][buffer->packet_id][modem_index];
            file_ptr->write(reinterpret_cast<const char*>(&buffer->buffer->buffer_start_time),
                            sizeof(double));

            if (dir == Direction::IN)
            {
                netsim::protobuf::LoggerEvent event;
                event.set_event(netsim::protobuf::LoggerEvent::PACKET_START);
                event.set_packet_id(buffer->packet_id);
                event.set_tx_modem_id(modem_index);
                interprocess().publish<netsim::groups::logger_event>(event);
            }
        }

        auto& file_ptr = files_[dir][buffer->packet_id][modem_index];
        if (file_ptr)
            file_ptr->write(reinterpret_cast<const char*>(&buffer->buffer->samples[0]),
                            buffer->buffer->samples.size() * sizeof(netsim::sample_t));
        //	else
        //	    glog.is(WARN) && glog << "No netsim::TaggedAudioBuffer::Marker::START so cannot log to file. Modem " << modem_index << ", dir: " << dir_to_str(dir)  << std::endl;

        // cleanly close out file
        if (buffer->marker == netsim::TaggedAudioBuffer::Marker::END)
        {
            files_[dir][buffer->packet_id].erase(modem_index);

            if (dir == Direction::OUT && files_[Direction::OUT][buffer->packet_id].empty())
            {
                netsim::protobuf::LoggerEvent event;
                event.set_event(netsim::protobuf::LoggerEvent::ALL_LOGS_CLOSED_FOR_PACKET);
                event.set_log_dir(cfg().logger().log_directory());
                std::stringstream ss_time;
                ss_time << start_time;
                event.set_start_time(ss_time.str());
                event.set_packet_id(buffer->packet_id);
		event.set_tx_modem_id(from_modem_index);

                interprocess().publish<netsim::groups::logger_event>(event);
            }
        }
    }

    std::string dir_to_str(Direction dir) { return (dir == Direction::IN) ? "IN" : "OUT"; }

  private:
    // map direction to packet_id to input/output modem_id
    std::map<Direction, std::map<int, std::map<int, std::unique_ptr<std::ofstream>>>> files_;
    std::string start_time{goby::time::file_str()};
};

#endif
