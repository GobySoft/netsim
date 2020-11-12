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
