// Copyright 2020:
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

#include "netsim/core/processor.h"
#include "processor_config.pb.h"

extern "C"
{
    void netsim_launch_processor_thread(
        goby::zeromq::MultiThreadApplication<netsim::protobuf::NetSimCoreConfig>* handler,
        int output_index);
}

template <int to_index> class ProcessorThread : public netsim::ProcessorThreadBase<to_index>
{
  public:
    ProcessorThread(const netsim::protobuf::NetSimCoreConfig& config)
        : netsim::ProcessorThreadBase<to_index>(config, 0 * boost::units::si::hertz),
          processor_config_(config.GetExtension(netsim::processor_echo))
    {
        goby::glog.is_debug1() && goby::glog << "Example cfg value: "
                                             << processor_config_.example_cfg_value() << std::endl;

        ++netsim::processor_ready;
    }

    void update_buffer_size(const jack_nframes_t& buffer_size) override
    {
        blocksize_ = buffer_size;
    }

    void detector_audio(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer,
                        int modem_id) override
    {
        // echo back data
        this->publish_audio_buffer(buffer, modem_id);
    }

  private:
    int blocksize_{0};
    const netsim::ProcessorEchoConfig& processor_config_;
};
