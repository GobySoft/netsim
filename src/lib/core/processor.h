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

#ifndef PROCESSOR20170816H
#define PROCESSOR20170816H

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "common.h"
#include "netsim/messages/core_config.pb.h"
#include "netsim/messages/groups.h"
#include <jack/types.h>

using ThreadBase = goby::middleware::SimpleThread<netsim::protobuf::NetSimCoreConfig>;

namespace netsim
{

class ProcessorThreadBase : public ThreadBase
{
  public:
    ProcessorThreadBase(const netsim::protobuf::NetSimCoreConfig& config, int index)
        : ThreadBase(config,
                     config.processor().impulse_response_update_hertz() * boost::units::si::hertz,
                     index)
    {
        // update buffer (audio block) size
        interthread().subscribe<netsim::groups::buffer_size_change, jack_nframes_t>(
            [this](const jack_nframes_t& buffer_size) { this->update_buffer_size(buffer_size); });

        // subscribe to all the detectors except our own id, since we ignore our transmissions
        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            auto detector_group_name = std::string("detector_audio_tx_") + std::to_string(i);
            detector_audio_groups_.push_back(goby::middleware::DynamicGroup(detector_group_name));

            auto audio_out_group_name = std::string("audio_out_from_") + std::to_string(i) +
                                        std::string("_to_") + std::to_string(ThreadBase::index());
            audio_out_groups_.push_back(goby::middleware::DynamicGroup(audio_out_group_name));

            // don't subscribe to our own audio
            if (i != ThreadBase::index())
            {
                auto detector_audio_callback =
                    [this, i](std::shared_ptr<const netsim::TaggedAudioBuffer> buffer) {
                        this->detector_audio(buffer, i);
                    };
                interthread().subscribe_dynamic<netsim::TaggedAudioBuffer>(detector_audio_callback,
                                                                   detector_audio_groups_[i]);
            }
        }
    }

    static std::atomic<int> ready;

  protected:
    virtual void detector_audio(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer,
                                int modem_index) = 0;

    virtual void update_buffer_size(const jack_nframes_t& buffer_size) = 0;

    goby::middleware::DynamicGroup& audio_out_group(int tx_modem_id)
    {
        return audio_out_groups_[tx_modem_id];
    }

  private:
    // indexed on tx modem id
    std::vector<goby::middleware::DynamicGroup> detector_audio_groups_;
    // indexed on tx modem id
    std::vector<goby::middleware::DynamicGroup> audio_out_groups_;
};
}

std::atomic<int> netsim::ProcessorThreadBase::ready{0};

#endif
