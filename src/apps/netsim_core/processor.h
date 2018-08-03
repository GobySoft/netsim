#ifndef PROCESSOR20170816H
#define PROCESSOR20170816H

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "jack_thread.h"

#include "lamss/lib_henrik_util/CConvolve.h"

using ThreadBase = goby::SimpleThread<NetSimCoreConfig>;

class ProcessorThread : public ThreadBase
{
public:
ProcessorThread(const NetSimCoreConfig& config, int index)
    : ThreadBase(config, 1.0*boost::units::si::hertz, index)
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

	if(cfg().continuous())
	{
	    interprocess().subscribe<groups::impulse_response, ImpulseResponse>(
		[this](const ImpulseResponse& r)
		{ if(r.receiver() == cfg().node_name(ThreadBase::index())) impulse_response_continuous(r); }
		);
	}
	else
	{
	    interprocess().subscribe<groups::impulse_response, ImpulseResponse>(
		[this](const ImpulseResponse& r)
		{ if(r.receiver() == cfg().node_name(ThreadBase::index())) impulse_response_discrete(r); }
		);
	}
	
	
	++ready;
    }

    void loop() override
    {
	// request all transmitters' impulse responses
	if(cfg().continuous())
	{
	    static std::atomic<int> imp_req_id(0);	    
	    for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	    {
		if(i != ThreadBase::index())
		{		    
		    ImpulseRequest imp_req;
		    imp_req.set_request_time(goby::common::goby_time<double>());
		    imp_req.set_request_id(imp_req_id++);
		    imp_req.set_source(cfg().node_name(i));
		    imp_req.set_receiver(cfg().node_name(ThreadBase::index()));
		    interprocess().publish<groups::impulse_request>(imp_req);
		}
	    }
	}
    }

    std::tuple<bool, int> find_index_from_source(const std::string& source)
    {
        bool found_index = false;
        int modem_index = 0;
        for(int n = cfg().node_name_size(); modem_index < n; ++modem_index)
        {
            if(cfg().node_name(modem_index) == source)
            {
                found_index = true;
                break;
            }
        }
	return std::make_tuple(found_index, modem_index);
    }
    
    void impulse_response_continuous(ImpulseResponse impulse_response)
    {
	using goby::glog; using namespace goby::common::logger;
        bool found_index = false;
        int modem_index = 0;
	std::tie(found_index, modem_index) = find_index_from_source(impulse_response.source());
	
        
        if(!found_index)
        {
            glog.is(WARN) && glog << "Processor Thread (" << ThreadBase::index() << ") Received impulse response from unknown source: " << impulse_response.source() << std::endl;
            return;
        }       
	else if(impulse_response.raytrace().size() == 0)
	{
//	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << ") Ignoring empty raytrace from: " << impulse_response.source() << " to " << impulse_response.receiver() << std::endl;
	    return;
	}

	if(cfg().processor().test_mode())
	{
	    impulse_response.clear_raytrace();
	    {
	        auto* raytrace = impulse_response.add_raytrace();
	        raytrace->add_element()->set_delay(.5);
	        raytrace->set_amplitude(1);
	    }
	}
	
	impulse_responses_[modem_index] = impulse_response;	
	if(convolve_[modem_index])
	    convolve_[modem_index]->update_ray_table(impulse_response);
    }
	
    void impulse_response_discrete(ImpulseResponse impulse_response)
    {
        using goby::glog; using namespace goby::common::logger;

        /* ImpulseResponse impulse_response; */
        
        /* impulse_response.set_source(actual_impulse_response.source()); */
        /* impulse_response.set_receiver(actual_impulse_response.receiver()); */
        /* impulse_response.set_request_id(actual_impulse_response.request_id()); */
        /* impulse_response.set_request_time(actual_impulse_response.request_time()); */
        
        /* { */
        /*     auto* raytrace = impulse_response.add_raytrace(); */
        /*     raytrace->set_delay(.5); */
        /*     raytrace->set_amplitude(1); */
        /* } */
        /* { */
        /*     auto* raytrace = impulse_response.add_raytrace(); */
        /*     raytrace->set_delay(3); */
        /*     raytrace->set_amplitude(0.5); */
        /* } */
        
        bool found_index = false;
        int modem_index = 0;
	std::tie(found_index, modem_index) = find_index_from_source(impulse_response.source());
        
        if(!found_index)
        {
            glog.is(WARN) && glog << "Processor Thread (" << ThreadBase::index() << ") Received impulse response from unknown source: " << impulse_response.source() << std::endl;
            return;
        }
        else if(audio_buffer_[impulse_response.request_id()].empty())
        {
            glog.is(WARN) && glog << "Processor Thread (" << ThreadBase::index() << ") Empty audio buffer for this request_id" << std::endl;
            return;
        }
	else if(impulse_response.raytrace().size() == 0)
	{
//	    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << ") Ignoring empty raytrace from: " << impulse_response.source() << " to " << impulse_response.receiver() << std::endl;
	    return;	    
	}
        
        glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): time: " << std::setprecision(15) << goby::common::goby_time<double>() << ", Received ImpulseResponse: " << impulse_response.DebugString() << std::endl;
        
        
        convolve_[impulse_response.request_id()].reset(new CConvolve);
        double timestamp;
            
        ArrayGain array_gain;

	if(cfg().processor().test_mode())
	{
	    impulse_response.clear_raytrace();
	    {
	        auto* raytrace = impulse_response.add_raytrace();
	        raytrace->add_element()->set_delay(.5);
	        raytrace->set_amplitude(1);
	    }
	}
	
        auto& full_signal = full_signal_[impulse_response.request_id()];
        auto& convolve = convolve_.at(impulse_response.request_id());
        convolve->initialize(blocksize_,
                             impulse_response.request_time(),
                             cfg().sampling_freq(),
                             cfg().processor().noise_level(),
                             cfg().processor().acomms_frequency(),
                             cfg().processor().surface_rms_roughness(),
                             cfg().processor().surface_roughness_loss(),
                             cfg().processor().source_calibration_db(),
                             cfg().processor().receiver_calibration_db(),
                             impulse_response,
                             array_gain,
                             timestamp,
                             full_signal);


        auto& audio_buffer = audio_buffer_[impulse_response.request_id()];

        // start frame
        detector_audio(audio_buffer.front(), modem_index, timestamp);
        audio_buffer.pop_front();

        // remaining frames
        for(auto buffer : audio_buffer)
            detector_audio(buffer, modem_index);

        audio_buffer_.erase(impulse_response.request_id());
    }
    
    void detector_audio(std::shared_ptr<const TaggedAudioBuffer> buffer, int modem_index, double receive_start_time = 0)
    {
	using goby::glog; using namespace goby::common::logger;

	if(cfg().continuous())
	{
	    double buffer_size = blocksize_;
	    double sampling_freq = cfg().sampling_freq();
	    
	    if(!convolve_[modem_index] || buffer->marker == TaggedAudioBuffer::Marker::START)
	    {
		ArrayGain array_gain;
		double dummy_timestamp;
		std::vector<std::vector<double> > dummy_signal_block;
		
		convolve_[modem_index].reset(new CConvolve);
		convolve_[modem_index]->initialize(blocksize_,
						   buffer->buffer->buffer_start_time,
						   cfg().sampling_freq(),
						   cfg().processor().noise_level(),
						   cfg().processor().acomms_frequency(),
						   cfg().processor().surface_rms_roughness(),
						   cfg().processor().surface_roughness_loss(),
						   cfg().processor().source_calibration_db(),
						   cfg().processor().receiver_calibration_db(),
						   impulse_responses_[modem_index],
						   array_gain,
						   dummy_timestamp,
						   dummy_signal_block);

		convolve_[modem_index]->set_signal_timestamp(buffer->buffer->buffer_start_time +
							     (buffer_size / sampling_freq)*cfg().jack().expected_number_buffer_delay());		   
	    }
	    
	    auto& convolve = convolve_[modem_index];		

	    std::shared_ptr<TaggedAudioBuffer> new_tagged_buffer(new TaggedAudioBuffer(*buffer));

	    // if test mode, just echo back data
	    // otherwise, actually run convolution
//	    if(!cfg().processor().test_mode())
//	    {
		    
		double signal_timestamp;
		    
		std::vector<double> replica_buffer(buffer->buffer->samples.begin(),
						   buffer->buffer->samples.end());
		    
		std::vector<std::vector<double> > signal_block(1);
		convolve->next_signal_block(signal_timestamp, noise_, replica_buffer,
					    signal_block);
		
		std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(signal_block.at(0).begin(), signal_block.at(0).end()));
//		new_audio_buffer->jack_frame_time = buffer->buffer->jack_frame_time + (signal_timestamp-buffer->buffer->buffer_start_time)*sampling_freq;
		new_audio_buffer->jack_frame_time = buffer->buffer->jack_frame_time + cfg().jack().expected_number_buffer_delay()*buffer_size;
		new_audio_buffer->buffer_start_time = signal_timestamp;

		new_tagged_buffer->buffer = new_audio_buffer;
//	    }
//	    else
//	    {
//		std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(buffer->buffer->samples.begin(), buffer->buffer->samples.end()));
//		new_audio_buffer->jack_frame_time = buffer->buffer->jack_frame_time + cfg().jack().expected_number_buffer_delay()*buffer_size;

//		new_tagged_buffer->buffer = new_audio_buffer;
//		
//	    }
		
	    interthread().publish_dynamic(new_tagged_buffer, audio_out_groups_[modem_index]);
	}
	else
	{	
	    // buffer while waiting on IMPULSE_RESPONSE
	    if(!convolve_[buffer->packet_id])
	    {
		if(buffer->marker == TaggedAudioBuffer::Marker::START)
		{
		    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received START buffer (id: " << buffer->packet_id << ", time: " << std::setprecision(15) << buffer->buffer->buffer_start_time << ", delay: " << (goby::common::goby_time<double>()-buffer->buffer->buffer_start_time) << ") of size: " << buffer->buffer->samples.size() << " from transmitter modem: " << modem_index << std::endl;
                
		    ImpulseRequest imp_req;
		    imp_req.set_request_time(buffer->buffer->buffer_start_time);
		    imp_req.set_request_id(buffer->packet_id);
		    imp_req.set_source(cfg().node_name(modem_index));
		    imp_req.set_receiver(cfg().node_name(ThreadBase::index()));
		    interprocess().publish<groups::impulse_request>(imp_req);
		}
		else if(audio_buffer_[buffer->packet_id].empty())
		{
		    glog.is(WARN) && glog << "Processor Thread (" << ThreadBase::index() << ", missed start of packet)" << std::endl;
		    return;
		}
            
		audio_buffer_[buffer->packet_id].push_back(buffer);
	    }
	    else
	    {
		std::shared_ptr<TaggedAudioBuffer> new_tagged_buffer(new TaggedAudioBuffer(*buffer));
		std::vector<double> double_buffer(buffer->buffer->samples.begin(), buffer->buffer->samples.end());
        
		auto& convolve = convolve_.at(buffer->packet_id);
		auto& full_signal = full_signal_.at(buffer->packet_id);
		const auto& noise = noise_;
		auto previous_end = full_signal.at(0).size();

//	    auto convolve_start_time = goby::common::goby_time<double>();
		convolve->signal_block(double_buffer, noise, full_signal);
//	    auto convolve_end_time = goby::common::goby_time<double>();
//	    glog.is(DEBUG1) && glog << "Signal block took: " << convolve_end_time - convolve_start_time << " seconds." << std::endl;

		if(buffer->marker == TaggedAudioBuffer::Marker::END)
		{
		    glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received END buffer (id: " << buffer->packet_id << ", (time: " << std::setprecision(15) << buffer->buffer->buffer_start_time << ", delay: " << (goby::common::goby_time<double>()-buffer->buffer->buffer_start_time) << ") of size: " << buffer->buffer->samples.size() << " from transmitter modem: " << modem_index << std::endl;

		    convolve->finalize(noise, full_signal);
	    
		    //std::stringstream file;
		    //file << "/tmp/convolve_full_buffer_" << buffer->packet_id;	  
		    //std::ofstream ofs(file.str().c_str(), std::ios::out | std::ios::binary);
		    //ofs.write(reinterpret_cast<const char*>(&buffer->buffer->buffer_start_time), sizeof(double));
		    //ofs.write(reinterpret_cast<const char*>(&full_signal_[0]), full_signal_.size()*sizeof(double));
		}            
            
		std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(full_signal.at(0).begin() + previous_end, full_signal.at(0).end()));

		if(buffer->marker == TaggedAudioBuffer::Marker::START)
		    new_audio_buffer->buffer_start_time = receive_start_time;

		new_tagged_buffer->buffer = new_audio_buffer;    

		if(buffer->marker == TaggedAudioBuffer::Marker::END)
		{
		    full_signal_.erase(buffer->packet_id);
		    convolve_.erase(buffer->packet_id);
		    audio_buffer_.erase(buffer->packet_id);
		}

		// process audio
	
		// test - write back with a 1 second delay to "our" modem
		//std::shared_ptr<TaggedAudioBuffer> newbuffer(new TaggedAudioBuffer(*buffer));
		//std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(*(buffer->buffer)));
		//new_audio_buffer->buffer_start_time += 0.5;
		//newbuffer->buffer = new_audio_buffer;

	
		interthread().publish_dynamic(new_tagged_buffer, audio_out_groups_[modem_index]);
	    }
	}
    }
    
    static std::atomic<int> ready;
private:
    // indexed on tx modem id
    std::vector<goby::DynamicGroup> detector_audio_groups_;
    // indexed on tx modem id
    std::vector<goby::DynamicGroup> audio_out_groups_;

    jack_nframes_t frames_since_silence{0};

    std::vector<double> noise_;
    // map of packet id to convolve (discrete mode) OR
    // map of tx modem id to convolve (continuous mode)
    std::map<int, std::unique_ptr<CConvolve>> convolve_;

    // map of packet id to signal (discrete mode)
    std::map<int, std::vector<std::vector<double>>> full_signal_;


    // map of packet id to buffer (while waiting for ImpulseResponse) (discrete mode)
    std::map<int, std::deque<std::shared_ptr<const TaggedAudioBuffer>>> audio_buffer_;

    int blocksize_{0};
    
    // map of tx modem id to latest impulse response
    std::map<int, ImpulseResponse> impulse_responses_;   
};

std::atomic<int> ProcessorThread::ready{0};


#endif
