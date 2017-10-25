#ifndef PROCESSOR20170816H
#define PROCESSOR20170816H

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "jack_thread.h"

#include "lamss/lib_henrik_util/CConvolve.h"

using ThreadBase = goby::SimpleThread<ModemSimConfig>;

class ProcessorThread : public ThreadBase
{
public:
ProcessorThread(const ModemSimConfig& config, int index)
    : ThreadBase(config, 0, index)
    {
	// generate 10 seconds of noise
	CConvolve<sample_t> temp;
	temp.create_noise(10.0*cfg().sampling_freq(), noise_);  

        // subscribe to all the detectors except our own id, since we ignore our transmissions
	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    auto detector_group_name = std::string("detector_audio_tx_") + std::to_string(i);
	    detector_audio_groups_.push_back(goby::DynamicGroup(detector_group_name));
	    
	    auto audio_out_group_name = std::string("audio_out_") + std::to_string(i);
	    audio_out_groups_.push_back(goby::DynamicGroup(audio_out_group_name));

	    // don't subscribe to our own audio
	    if(i != ThreadBase::index())
	    {		
		auto detector_audio_callback = [this, i](std::shared_ptr<const AudioBuffer> buffer) { this->detector_audio(buffer, i); };
		interthread().subscribe_dynamic<AudioBuffer>(detector_audio_callback, detector_audio_groups_[i]);
	    }
	}		       
    }

    void detector_audio(std::shared_ptr<const AudioBuffer> buffer, int modem_index)
    {
	using goby::glog; using namespace goby::common::logger;

	if(buffer->marker == AudioBuffer::Marker::START)
	{
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received START buffer (time: " << std::setprecision(15) << buffer->buffer_start_time << ", delay: " << (goby::common::goby_time<double>()-buffer->buffer_start_time) << ") of size: " << buffer->samples.size() << " from transmitter modem: " << modem_index << std::endl;

	    convolve_.reset(new CConvolve<sample_t>);

	    double timestamp;
	    
	    /* convolve_->initialize(blocksize,transmission[i].timestamp, */
	    /* 			  cfg().sampling_freq(), */
	    /* 			  noise_level, */
	    /* 			  cfg().source_calibration_db(), */
	    /* 			  cfg().receiver_calibration_db(), */
	    /* 			  impulse_response, */
	    /* 			  array_gain, */
	    /* 			  timestamp,signal); */
	}
	else if(buffer->marker == AudioBuffer::Marker::END)
	{
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received END buffer (time: " << std::setprecision(15) << buffer->buffer_start_time << ", delay: " << (goby::common::goby_time<double>()-buffer->buffer_start_time) << ") of size: " << buffer->samples.size() << " from transmitter modem: " << modem_index << std::endl;
	}


        // process audio
	
	// test - write back with a 1 second delay to "our" modem
	std::shared_ptr<AudioBuffer> newbuffer(new AudioBuffer(*buffer));
	newbuffer->buffer_start_time += 0.0;
	interthread().publish_dynamic(newbuffer, audio_out_groups_[ThreadBase::index()]);
    }

private:
    std::vector<goby::DynamicGroup> detector_audio_groups_;
    std::vector<goby::DynamicGroup> audio_out_groups_;

    bool in_packet_{false};
    jack_nframes_t frames_since_silence{0};

    std::vector<sample_t> noise_;
    std::unique_ptr<CConvolve<sample_t>> convolve_;
};

#endif
