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

        interprocess().subscribe<groups::impulse_response, ImpulseResponse>(
            [this](const ImpulseResponse& r)
            { if(r.receiver() == cfg().node_name(ThreadBase::index())) impulse_response(r); }
            );
        
	++ready;
    }

    void impulse_response(const ImpulseResponse& impulse_response)
    {
        using goby::glog; using namespace goby::common::logger;

        bool found_index = false;
        int modem_index = 0;
        for(int n = cfg().node_name_size(); modem_index < n; ++modem_index)
        {
            if(cfg().node_name(modem_index) == impulse_response.source())
            {
                found_index = true;
                break;
            }
        }

        if(!found_index)
        {
            glog.is(WARN) && glog << "Received impulse response from unknown source: " << impulse_response.source() << std::endl;
            return;
        }
        else if(audio_buffer_[impulse_response.request_id()].empty())
        {
            glog.is(WARN) && glog << "Empty audio buffer for this request_id" << std::endl;
            return;
        }
        
        
        glog.is(DEBUG1) && glog << "Processor Thread (" << ThreadBase::index() << "): Received ImpulseResponse: " << impulse_response.ShortDebugString() << ", time: " << std::setprecision(15) << goby::common::goby_time<double>() << " from transmitter modem: " << modem_index << std::endl;
        
        convolve_[impulse_response.request_id()].reset(new CConvolve);
        double timestamp;
            
        ArrayGain array_gain;

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
            auto previous_end = full_signal.size();
            convolve->signal_block(double_buffer, noise, full_signal);

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
            
            std::shared_ptr<AudioBuffer> new_audio_buffer(new AudioBuffer(full_signal.begin() + previous_end, full_signal.end()));

            if(buffer->marker == TaggedAudioBuffer::Marker::START)
                new_audio_buffer->buffer_start_time = receive_start_time;

            new_tagged_buffer->buffer = new_audio_buffer;    

            if(buffer->marker == TaggedAudioBuffer::Marker::END)
            {
                full_signal_.erase(buffer->packet_id);
                convolve_.erase(buffer->packet_id);
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


    // map of packet id to buffer (which waiting for ImpulseResponse)
    std::map<int, std::deque<std::shared_ptr<const TaggedAudioBuffer>>> audio_buffer_;

    int blocksize_{0};

};

std::atomic<int> ProcessorThread::ready{0};


#endif
