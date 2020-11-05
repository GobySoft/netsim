#include "netsim/core/processor.h"

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
        : netsim::ProcessorThreadBase<to_index>(config)
    {
        ++netsim::processor_ready;
    }

    void loop() override {}

    void update_buffer_size(const jack_nframes_t& buffer_size) override
    {
        blocksize_ = buffer_size;
    }

    void detector_audio(std::shared_ptr<const netsim::TaggedAudioBuffer> buffer, int modem_id) override
    {
        // echo back data
        this->publish_audio_buffer(buffer, modem_id);
    }

  private:
    int blocksize_{0};
};
