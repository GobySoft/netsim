#ifndef DETECTOR20170816H
#define DETECTOR20170816H

#include <boost/circular_buffer.hpp>

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "jack_thread.h"

using ThreadBase = goby::SimpleThread<ModemSimConfig>;

class DetectorThread : public ThreadBase
{
public:

DetectorThread(const ModemSimConfig& config, int index)
    : ThreadBase(config, 0, index),
	audio_in_group_(std::string("audio_in_") + std::to_string(ThreadBase::index())),
	detector_audio_group_(std::string("detector_audio_tx_") + std::to_string(ThreadBase::index()))
    {
	using goby::glog; using namespace goby::common::logger;	

	auto audio_in_callback = [this](std::shared_ptr<const AudioBuffer> buffer) { this->audio_in(buffer); };

	interthread().subscribe_dynamic<AudioBuffer>(audio_in_callback, audio_in_group_);
	interthread().subscribe<groups::buffer_size_change, jack_nframes_t>(
	    [this](const jack_nframes_t& buffer_size)
	    {
		size_t new_prebuffer_size = (cfg().sampling_freq()*cfg().detector().packet_begin_prebuffer_seconds())/buffer_size;
		glog.is(VERBOSE) && glog << "Resizing prebuffer to: " << new_prebuffer_size << std::endl;
		prebuffer_.resize(new_prebuffer_size > 0 ? new_prebuffer_size : 1);
	    });
	
		
	glog.is(VERBOSE) && glog << "Starting detector thread: " << ThreadBase::index() << std::endl;
       
    }

    void audio_in(std::shared_ptr<const AudioBuffer> buffer)
    {
	using goby::glog; using namespace goby::common::logger;
	       
	glog.is(DEBUG2) && glog << "Detector Thread (" << ThreadBase::index() << "): Received buffer of size: " << buffer->samples.size() << std::endl;


	if(!in_packet_)
	{
	    prebuffer_.push_back(buffer);
	    for(auto it = buffer->samples.begin(), end = buffer->samples.end(); it != end; ++it)
	    {
		if(std::abs(*it) >= cfg().detector().detection_threshold())
		{
		    // send subbuffer from start of packet to end of buffer
		    in_packet_ = true;
		    auto& first_buffer = prebuffer_.front();
		    std::shared_ptr<AudioBuffer> subbuffer(new AudioBuffer(first_buffer->samples.begin()+(it-buffer->samples.begin()), first_buffer->samples.end()));
		    subbuffer->marker = AudioBuffer::Marker::START;
		    subbuffer->buffer_start_time = buffer->buffer_start_time + static_cast<double>(it - buffer->samples.begin())/static_cast<double>(cfg().sampling_freq());
		    interthread().publish_dynamic(subbuffer, detector_audio_group_);
		    prebuffer_.pop_front();

		    while(!prebuffer_.empty())
		    {
			interthread().publish_dynamic(prebuffer_.front(), detector_audio_group_);
			prebuffer_.pop_front();
		    }
		    // assume a packet spans at least one buffer, so no need to check the rest of the buffer
		    glog.is(DEBUG1) && glog << "Detector Thread (" << ThreadBase::index() << "): Detected START at time: " << std::setprecision(15) << subbuffer->buffer_start_time << std::endl;

		    break;
		}	       
	    }
	}
	else
	{
	    for(auto it = buffer->samples.begin(), end = buffer->samples.end(); it != end; ++it)
	    {
		if(std::abs(*it) >= cfg().detector().detection_threshold())
		    frames_since_silence = 0;
		else
		    ++frames_since_silence;

		if(frames_since_silence >= cfg().detector().packet_end_silent_seconds()*cfg().sampling_freq())
		{
		    // send subbuffer from beginning of the buffer to the
		    // end of the silent period following packet
		    in_packet_ = false;
		    frames_since_silence = 0;
		    std::shared_ptr<AudioBuffer> subbuffer(new AudioBuffer(buffer->samples.begin(), it));
		    subbuffer->marker = AudioBuffer::Marker::END;
		    subbuffer->buffer_start_time = buffer->buffer_start_time;
		    interthread().publish_dynamic(subbuffer, detector_audio_group_);
		    glog.is(DEBUG1) && glog << "Detector Thread (" << ThreadBase::index() << "): Detected END at time: " << std::setprecision(15) << subbuffer->buffer_start_time+static_cast<double>(it-buffer->samples.begin())/static_cast<double>(cfg().sampling_freq()) << std::endl;

		    // assume no new packet starts within the same buffer
		    break;
		}
	    }
	    if(in_packet_)
	    {
		// send whole buffer
		interthread().publish_dynamic(buffer, detector_audio_group_);	    
	    }	   
	}       
    }

private:  
    goby::DynamicGroup audio_in_group_;
    goby::DynamicGroup detector_audio_group_;

    bool in_packet_{false};
    jack_nframes_t frames_since_silence{0};

    boost::circular_buffer<std::shared_ptr<const AudioBuffer>> prebuffer_{0};
};

#endif
