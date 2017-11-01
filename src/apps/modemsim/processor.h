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
	interthread().subscribe<groups::buffer_size_change, jack_nframes_t>(
	    [this](const jack_nframes_t& buffer_size)
	    { blocksize_ = buffer_size; });

        // generate 10 seconds of noise
	CConvolve temp;
	temp.create_noise(10.0*cfg().sampling_freq(), noise_);  

        // subscribe to all the detectors except our own id, since we ignore our transmissions
	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    auto detector_group_name = std::string("detector_audio_tx_") + std::to_string(i);
	    detector_audio_groups_.push_back(goby::DynamicGroup(detector_group_name));
	    
	    auto audio_out_group_name = std::string("audio_out_from_") + std::to_string(i) + std::string("_to_") + std::to_string(ThreadBase::index());	   
	    audio_out_groups_.push_back(goby::DynamicGroup(audio_out_group_name));

	    // don't subscribe to our own audio
	    if(i != ThreadBase::index())
	    {		
		auto detector_audio_callback = [this, i](std::shared_ptr<const TaggedAudioBuffer> buffer) { this->detector_audio(buffer, i); };
		interthread().subscribe_dynamic<TaggedAudioBuffer>(detector_audio_callback, detector_audio_groups_[i]);
	    }
	}
	++ready;
    }

    void detector_audio(std::shared_ptr<const TaggedAudioBuffer> buffer, int modem_index)
    {
	using goby::glog; using namespace goby::common::logger;
	
	std::shared_ptr<TaggedAudioBuffer> new_tagged_buffer(new TaggedAudioBuffer(*buffer));
	std::vector<double> double_buffer(buffer->buffer->samples.begin(), buffer->buffer->samples.end());
	if(buffer->marker == TaggedAudioBuffer::Marker::START)
	{
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received START buffer (id: " << buffer->packet_id << ", time: " << std::setprecision(15) << buffer->buffer->buffer_start_time << ", delay: " << (goby::common::goby_time<double>()-buffer->buffer->buffer_start_time) << ") of size: " << buffer->buffer->samples.size() << " from transmitter modem: " << modem_index << std::endl;
	    
	    convolve_[buffer->packet_id].reset(new CConvolve);
	    auto& convolve = convolve_.at(buffer->packet_id);
	    auto& full_signal = full_signal_[buffer->packet_id];
	    const auto& noise = noise_;
	    
	    double timestamp;

	    ImpulseResponse impulse_response;
	    impulse_response.set_source("src");
	    impulse_response.set_source("dst");
	    {
		auto* raytrace = impulse_response.add_raytrace();
		raytrace->set_delay(.5);
		raytrace->set_amplitude(1);
	    }
//	    {
//		auto* raytrace = impulse_response.add_raytrace();
//		raytrace->set_delay(3);
//		raytrace->set_amplitude(0.5);
//	    }
	    
	    ArrayGain array_gain;

	    convolve->initialize(blocksize_,
	    			  buffer->buffer->buffer_start_time,
	    			  cfg().sampling_freq(),
	    			  cfg().processor().noise_level(),
	    			  cfg().processor().source_calibration_db(),
	    			  cfg().processor().receiver_calibration_db(),
	    			  impulse_response,
	    			  array_gain,
	    			  timestamp,
				  full_signal);
	    convolve->signal_block(double_buffer, noise, full_signal);
	    
	    std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(full_signal.begin(), full_signal.end()));
	    new_audio_buffer->buffer_start_time = timestamp;
	    new_tagged_buffer->buffer = new_audio_buffer;    
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "), writing buffer of size: " << new_tagged_buffer->buffer->samples.size() << std::endl;
	}
	else if(buffer->marker == TaggedAudioBuffer::Marker::MIDDLE)
	{
	    auto& convolve = convolve_.at(buffer->packet_id);
	    auto& full_signal = full_signal_.at(buffer->packet_id);
	    const auto& noise = noise_;
	    auto previous_end = full_signal.size();
	    convolve->signal_block(double_buffer, noise, full_signal);
	    std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(full_signal.begin() + previous_end, full_signal.end()));
	    new_tagged_buffer->buffer = new_audio_buffer;    
	}
	else if(buffer->marker == TaggedAudioBuffer::Marker::END)
	{
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received END buffer (id: " << buffer->packet_id << ", (time: " << std::setprecision(15) << buffer->buffer->buffer_start_time << ", delay: " << (goby::common::goby_time<double>()-buffer->buffer->buffer_start_time) << ") of size: " << buffer->buffer->samples.size() << " from transmitter modem: " << modem_index << std::endl;
	    auto& convolve = convolve_.at(buffer->packet_id);
	    auto& full_signal = full_signal_.at(buffer->packet_id);
	    auto previous_end = full_signal.size();
	    const auto& noise = noise_;
	    convolve->signal_block(double_buffer, noise, full_signal);
	    convolve->finalize(noise, full_signal);
	    std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(full_signal.begin() + previous_end, full_signal.end()));
	    new_tagged_buffer->buffer = new_audio_buffer;
	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "), writing buffer of size: " << new_tagged_buffer->buffer->samples.size() << std::endl;

	    convolve_.erase(buffer->packet_id);
	    full_signal_.erase(buffer->packet_id);
	    
	    //std::stringstream file;
	    //file << "/tmp/convolve_full_buffer_" << buffer->packet_id;	  
	    //std::ofstream ofs(file.str().c_str(), std::ios::out | std::ios::binary);
	    //ofs.write(reinterpret_cast<const char*>(&buffer->buffer->buffer_start_time), sizeof(double));
	    //ofs.write(reinterpret_cast<const char*>(&full_signal_[0]), full_signal_.size()*sizeof(double));
	}

        // process audio
	
	// test - write back with a 1 second delay to "our" modem
	//std::shared_ptr<TaggedAudioBuffer> newbuffer(new TaggedAudioBuffer(*buffer));
	//std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(*(buffer->buffer)));
	//new_audio_buffer->buffer_start_time += 0.5;
	//newbuffer->buffer = new_audio_buffer;

	
	interthread().publish_dynamic(new_tagged_buffer, audio_out_groups_[modem_index]);
    }
    
    static std::atomic<int> ready;
private:
    // indexed on tx modem id
    std::vector<goby::DynamicGroup> detector_audio_groups_;
    // indexed on tx modem id
    std::vector<goby::DynamicGroup> audio_out_groups_;

    bool in_packet_{false};
    jack_nframes_t frames_since_silence{0};

    std::vector<double> noise_;
    // map of packet id to convolve
    std::map<int, std::unique_ptr<CConvolve>> convolve_;

    // map of packet id to signal
    std::map<int, std::vector<double>> full_signal_;

    int blocksize_{0};

};

std::atomic<int> ProcessorThread::ready{0};


#endif
