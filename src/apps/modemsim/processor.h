#ifndef PROCESSOR20170816H
#define PROCESSOR20170816H

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "jack_thread.h"

using ThreadBase = goby::SimpleThread<ModemSimConfig>;

class ProcessorThread : public ThreadBase
{
public:
    using BufferType = JackThread::BufferType;

ProcessorThread(const ModemSimConfig& config, int index)
    : ThreadBase(config, 0, index)
    {
	// subscribe to all the detectors except our own id, since we ignore our transmissions
	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    if(i != ThreadBase::index())
	    {
		auto detector_group_name = std::string("detector_audio_tx_") + std::to_string(i);
		detector_audio_groups_.push_back(goby::DynamicGroup(detector_group_name));
		
		auto detector_audio_callback = [this, i](std::shared_ptr<const BufferType> buffer) { this->detector_audio(buffer, i); };
		interthread().subscribe_dynamic<BufferType>(detector_audio_callback, detector_audio_groups_.back());
	    }
	}		       
    }

    void detector_audio(std::shared_ptr<const BufferType> buffer, int modem_index)
    {
	using goby::glog; using namespace goby::common::logger;

	if(buffer->size() != 1024)
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received buffer of size: " << buffer->size() << " from transmitter modem: " << modem_index << std::endl;

    }

private:
    std::vector<goby::DynamicGroup> detector_audio_groups_;

    bool in_packet_{false};
    jack_nframes_t frames_since_silence{0};
};

#endif
