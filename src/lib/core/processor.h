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
extern std::atomic<int> processor_ready;

template <int to_index> class ProcessorThreadBase : public ThreadBase
{
  public:
    ProcessorThreadBase(const netsim::protobuf::NetSimCoreConfig& config)
        : ThreadBase(config,
                     config.processor().impulse_response_update_hertz() * boost::units::si::hertz)
    {
        // update buffer (audio block) size
        interthread().template subscribe<netsim::groups::buffer_size_change, jack_nframes_t>(
            [this](const jack_nframes_t& buffer_size) { this->update_buffer_size(buffer_size); });

        // subscribe to all the detectors except our own id, since we ignore our transmissions
        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            audio_out_groups_.push_back(goby::middleware::DynamicGroup("aout", i + to_index));

            // don't subscribe to our own audio
            if (i != to_index)
            {
                auto detector_audio_callback =
                    [this, i](std::shared_ptr<const netsim::TaggedAudioBuffer> buffer) {
                        this->detector_audio(buffer, i);
                    };
                switch (i)
                {
                    case 0:
                        interthread()
                            .template subscribe<netsim::groups::DetectorAudio<0>::group,
                                                netsim::TaggedAudioBuffer>(detector_audio_callback);
                        break;
                    case 1:
                        interthread()
                            .template subscribe<netsim::groups::DetectorAudio<1>::group,
                                                netsim::TaggedAudioBuffer>(detector_audio_callback);
                        break;
                    case 2:
                        interthread()
                            .template subscribe<netsim::groups::DetectorAudio<2>::group,
                                                netsim::TaggedAudioBuffer>(detector_audio_callback);
                        break;
                    case 3:
                        interthread()
                            .template subscribe<netsim::groups::DetectorAudio<3>::group,
                                                netsim::TaggedAudioBuffer>(detector_audio_callback);
                        break;
                }
            }
        }
    }

  protected:
    virtual void detector_audio(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer,
                                int modem_index) = 0;

    virtual void update_buffer_size(const jack_nframes_t& buffer_size) = 0;

    void publish_audio_buffer(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer,
                              int tx_modem_id)
    {
        switch (tx_modem_id)
        {
            case 0:
                this->interthread().template publish<netsim::groups::AudioOut<0, to_index>::group>(
                    buffer);
                break;
            case 1:
                this->interthread().template publish<netsim::groups::AudioOut<1, to_index>::group>(
                    buffer);
                break;
            case 2:
                this->interthread().template publish<netsim::groups::AudioOut<2, to_index>::group>(
                    buffer);
                break;
            case 3:
                this->interthread().template publish<netsim::groups::AudioOut<3, to_index>::group>(
                    buffer);
                break;
        }
    }

  private:
    // indexed on tx modem id
    std::vector<goby::middleware::DynamicGroup> audio_out_groups_;
};
} // namespace netsim

#endif
