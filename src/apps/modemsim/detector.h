#ifndef DETECTOR20170816H
#define DETECTOR20170816H

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "jack_thread.h"

using ThreadBase = goby::Thread<ModemSimConfig, goby::InterProcessForwarder<goby::InterThreadTransporter>>;

class DetectorThread : public ThreadBase
{
public:
    using BufferType = std::vector<std::pair<jack_nframes_t, JackThread::sample_t>>;      

DetectorThread(const ModemSimConfig& config, ThreadBase::Transporter* t, int index)
    : ThreadBase(config, t, index),
	audio_in_group_(std::string("audio_in_") + std::to_string(ThreadBase::index()))
    {
	using goby::glog; using namespace goby::common::logger;	

	auto audio_in_callback = [this](std::shared_ptr<const BufferType> buffer) { this->audio_in(*buffer); };

	transporter().inner().subscribe_dynamic<BufferType>(audio_in_callback, audio_in_group_);
	
	glog.is(VERBOSE) && glog << "Starting detector thread: " << ThreadBase::index() << std::endl;
       
    }

    void audio_in(const BufferType& buffer)
    {
	using goby::glog; using namespace goby::common::logger;

	glog.is(VERBOSE) && glog << "Detector Thread (" << ThreadBase::index() << "): Received buffer of size: " << buffer.size() << std::endl;
    }

private:
    goby::DynamicGroup audio_in_group_;
};

#endif
