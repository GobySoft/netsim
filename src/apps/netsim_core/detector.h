#ifndef DETECTOR20170816H
#define DETECTOR20170816H

#include <boost/circular_buffer.hpp>

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "jack_thread.h"

using ThreadBase = goby::SimpleThread<NetSimCoreConfig>;

class DetectorThread : public ThreadBase
{
public:

DetectorThread(const NetSimCoreConfig& config, int index)
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
	++ready;
    }

    void audio_in(std::shared_ptr<const AudioBuffer> buffer)
    {
	using goby::glog; using namespace goby::common::logger;
	       
	glog.is(DEBUG2) && glog << "Detector Thread (" << ThreadBase::index() << "): Received buffer of size: " << buffer->samples.size() << std::endl;

	static std::atomic<int> global_packet_id{0};

	if(!in_packet_)
	{
	    prebuffer_.push_back(buffer);
	    for(auto it = buffer->samples.begin(), end = buffer->samples.end(); it != end; ++it)
	    {
		if(std::abs(*it) >= cfg().detector().detection_threshold())
		{
		    // send subbuffer from start of packet to end of buffer
		    in_packet_ = true;
		    in_packet_id = global_packet_id++;

		    if(cfg().continuous())
		    {
			std::shared_ptr<TaggedAudioBuffer> tagged_buffer(new TaggedAudioBuffer);
			tagged_buffer->buffer = buffer;	    
			tagged_buffer->marker = TaggedAudioBuffer::Marker::START;
			tagged_buffer->packet_id = in_packet_id;
			interthread().publish_dynamic(tagged_buffer, detector_audio_group_);
		    }
		    else
		    {
			auto& first_buffer = prebuffer_.front();

			if(!first_buffer)
			    break;
			
			std::shared_ptr<AudioBuffer> subbuffer(new AudioBuffer(*first_buffer));

			std::shared_ptr<TaggedAudioBuffer> tagged_subbuffer(new TaggedAudioBuffer);
			tagged_subbuffer->buffer = subbuffer;
		    
			tagged_subbuffer->marker = TaggedAudioBuffer::Marker::START;
			tagged_subbuffer->packet_id = in_packet_id;
		    
			interthread().publish_dynamic(tagged_subbuffer, detector_audio_group_);
			prebuffer_.pop_front();

			while(!prebuffer_.empty())
			{
			    std::shared_ptr<TaggedAudioBuffer> tagged_buffer(new TaggedAudioBuffer);
			    tagged_buffer->buffer = prebuffer_.front();
			    tagged_buffer->packet_id = in_packet_id;
			    interthread().publish_dynamic(tagged_buffer, detector_audio_group_);
			    prebuffer_.pop_front();
			}
		    }

                    // assume a packet spans at least one buffer, so no need to check the rest of the buffer
		    glog.is(DEBUG1) && glog << "Detector Thread (" << ThreadBase::index() << "): Detected START at time: " << std::setprecision(15) << buffer->buffer_start_time << std::endl;

		    break;
		}	       
	    }

	    if(cfg().continuous() && !in_packet_)
	    {
		// send whole buffer
		std::shared_ptr<TaggedAudioBuffer> tagged_buffer(new TaggedAudioBuffer);
		tagged_buffer->buffer = buffer;	    
		tagged_buffer->packet_id = in_packet_id;
		interthread().publish_dynamic(tagged_buffer, detector_audio_group_);
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
		    std::shared_ptr<AudioBuffer> subbuffer(new AudioBuffer(*buffer));
		    subbuffer->buffer_start_time = buffer->buffer_start_time;

		    std::shared_ptr<TaggedAudioBuffer> tagged_subbuffer(new TaggedAudioBuffer);
		    tagged_subbuffer->buffer = subbuffer;		    
		    tagged_subbuffer->marker = TaggedAudioBuffer::Marker::END;
		    tagged_subbuffer->packet_id = in_packet_id;
		    
		    interthread().publish_dynamic(tagged_subbuffer, detector_audio_group_);
		    glog.is(DEBUG1) && glog << "Detector Thread (" << ThreadBase::index() << "): Detected END at time: " << std::setprecision(15) << subbuffer->buffer_start_time+static_cast<double>(it-buffer->samples.begin())/static_cast<double>(cfg().sampling_freq()) << std::endl;		   
		    // assume no new packet starts within the same buffer
		    break;
		}
	    }
	    if(in_packet_ || cfg().continuous())
	    {
		// send whole buffer
		std::shared_ptr<TaggedAudioBuffer> tagged_buffer(new TaggedAudioBuffer);
		tagged_buffer->buffer = buffer;	    
		if(in_packet_)
		    tagged_buffer->marker = TaggedAudioBuffer::Marker::MIDDLE;	       
		tagged_buffer->packet_id = in_packet_id;	       
		interthread().publish_dynamic(tagged_buffer, detector_audio_group_);
	    }
	}       
    }

    static std::atomic<int> ready;
private:  
    goby::DynamicGroup audio_in_group_;
    goby::DynamicGroup detector_audio_group_;

    bool in_packet_{false};
    int in_packet_id{-1};
    jack_nframes_t frames_since_silence{0};

    boost::circular_buffer<std::shared_ptr<const AudioBuffer>> prebuffer_{1};
};

std::atomic<int> DetectorThread::ready{0};

#endif
