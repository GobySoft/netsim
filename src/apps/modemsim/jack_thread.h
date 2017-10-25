#ifndef JACKTHREAD20170815H
#define JACKTHREAD20170815H

#include <iostream>
#include <unistd.h>
#include <jack/jack.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "common.h"
#include "goby/middleware/multi-thread-application.h"
#include "goby/common/time.h"
#include "config.pb.h"
#include "messages/groups.h"

using ThreadBase = goby::SimpleThread<ModemSimConfig>;

int jack_process(jack_nframes_t nframes, void* arg);
void jack_shutdown (void *arg);
int jack_xrun(void* arg);
int jack_buffer_size_change(jack_nframes_t nframes, void *arg);

class JackThread : public ThreadBase
{
public:
    
JackThread(const ModemSimConfig& config)
    : ThreadBase(config, ThreadBase::loop_max_frequency()),
	input_port_(config.number_of_modems(), nullptr),	
	output_port_(config.number_of_modems(), nullptr),
	fs_(config.sampling_freq())
    {
	using goby::glog; using namespace goby::common::logger;
	static_assert(sizeof(float) == 4, "Float must be 4 bytes");

	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    audio_in_groups_.push_back(goby::DynamicGroup(std::string("audio_in_") + std::to_string(i)));
	    audio_out_groups_.push_back(goby::DynamicGroup(std::string("audio_out_") + std::to_string(i)));
	    auto audio_out_callback = [this, i](std::shared_ptr<const TaggedAudioBuffer> buffer) {this->audio_out(buffer, i); };
	    interthread().subscribe_dynamic<TaggedAudioBuffer>(audio_out_callback, audio_out_groups_[i]);
	}
	
	const char *client_name = "modemsim";
	const char *server_name = nullptr;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	
	/* open a client connection to the JACK server */

	client_ = jack_client_open (client_name, options, &status, server_name);

	if (!client_)
	{
	    glog.is(WARN) && glog << "jack_client_open() failed, status = " << status << std::endl;
	    
	    if (status & JackServerFailed)
		glog.is(DIE) && glog << "Unable to connect to JACK server" << std::endl;
	    
	    exit(1);
	} 
 
	if (status & JackNameNotUnique)
	{
	    client_name = jack_get_client_name(client_);
	    glog.is(VERBOSE) && glog << "unique name " << client_name << " assigned." << std::endl;
	}
  
	jack_set_process_callback (client_, ::jack_process, this);
	jack_on_shutdown (client_, ::jack_shutdown, this);
	jack_set_xrun_callback (client_, ::jack_xrun, this);
	jack_set_buffer_size_callback(client_, ::jack_buffer_size_change, this);
    
	auto fs_actual = jack_get_sample_rate(client_);
	if(fs_actual != fs_)
	    glog.is(DIE) && glog << "Engine sample rate: " << fs_actual << ", this client requires: " << fs_ << std::endl;

	// Connect the ports.  You can't do this before the client is
	// activated, because we can't make connections to clients
	// that aren't running.  Note the confusing (but necessary)
	// orientation of the driver backend ports: playback ports are
	// "input" to the backend, and capture ports are "output" from
	// it.
	if (jack_activate (client_))
	    glog.is(DIE) && glog << "cannot activate client" << std::endl;


	const char** capture_port_names = jack_get_ports (client_, nullptr, nullptr,
							  JackPortIsPhysical|JackPortIsOutput);
	if (capture_port_names == nullptr)
	    glog.is(DIE) && glog <<  "no physical capture ports" << std::endl;

	int j = 0;
	while(capture_port_names[j])
	{
	    glog.is(DEBUG2) && glog << "Capture port: " << capture_port_names[j] << std::endl;
	    ++j;
	}

	
	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    std::string capture_port_name = cfg().jack().capture_port_prefix() + std::to_string(i+cfg().jack().port_name_starting_index());
	    
	    int j = 0;
	    bool found_name = false;
	    while(capture_port_names[j])
	    {
		if(std::string(capture_port_names[j++]) == capture_port_name)
		    found_name = true;
	    }
	    
	    if(!found_name)
		glog.is(DIE) && glog << "No capture port with name: " << capture_port_name << std::endl;
	    
	    std::string input_port_name = std::string("input_") + std::to_string(i);
	    input_port_[i] = jack_port_register (client_, input_port_name.c_str(),
						 port_type_.c_str(),
						 JackPortIsInput, 0);
	    if (!input_port_[i])
		glog.is(DIE) && glog << "no more JACK ports available" << std::endl;
	    
	    if (jack_connect (client_, capture_port_name.c_str(), jack_port_name (input_port_[i])))
		glog.is(DIE) && glog << "cannot connect input port: " << input_port_name << " to capture port: " << capture_port_name << std::endl;
	}
	free (capture_port_names);


	const char** playback_port_names = jack_get_ports (client_, nullptr, nullptr,
							  JackPortIsPhysical|JackPortIsInput);
	if (playback_port_names == nullptr)
	    glog.is(DIE) && glog <<  "no physical playback ports" << std::endl;

	j = 0;
	while(playback_port_names[j])
	{
	    glog.is(DEBUG2) && glog << "Playback port: " << playback_port_names[j] << std::endl;
	    ++j;
	}

	
	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    std::string playback_port_name = cfg().jack().playback_port_prefix() + std::to_string(i+cfg().jack().port_name_starting_index());
	    
	    int j = 0;
	    bool found_name = false;
	    while(playback_port_names[j])
	    {
		if(std::string(playback_port_names[j++]) == playback_port_name)
		    found_name = true;
	    }
	    
	    if(!found_name)
		glog.is(DIE) && glog << "No playback port with name: " << playback_port_name << std::endl;
	    
	    std::string output_port_name = std::string("output_") + std::to_string(i);
	    output_port_[i] = jack_port_register (client_, output_port_name.c_str(),
						 port_type_.c_str(),
						 JackPortIsOutput, 0);
	    if (!output_port_[i])
		glog.is(DIE) && glog << "no more JACK ports available" << std::endl;
	    
	    if (jack_connect (client_, jack_port_name (output_port_[i]), playback_port_name.c_str()))
		glog.is(DIE) && glog << "cannot connect output port: " << output_port_name << " to playback port: " << playback_port_name << std::endl;
	}
	free (playback_port_names);
    }
  
    ~JackThread()
    {
	jack_client_close (client_);
    }

    // The process callback for this JACK application is called in a
    // special realtime thread once for each audio cycle.
    int jack_process (jack_nframes_t nframes)
    {
	using goby::glog; using namespace goby::common::logger;
	double buffer_start_time = goby::common::goby_time<double>();
	auto process_frame_time = jack_last_frame_time(client_);

	for (int input_port_i = 0, n = cfg().number_of_modems(); input_port_i < n; ++input_port_i)
	{	    
	    std::shared_ptr<AudioBuffer> rx_buffer(new AudioBuffer(buffer_size_));
	    rx_buffer->buffer_start_time = buffer_start_time;
	    
	    if(input_port_[input_port_i] != nullptr) // could be briefly empty while we initialize all the threads
	    {
		sample_t* in = (sample_t*)jack_port_get_buffer (input_port_[input_port_i], nframes);
		sample_t* sample = in;
		for(jack_nframes_t frame = 0; frame < nframes; ++frame)
		{
		    (*rx_buffer).samples[frame] = *sample;
		    ++sample;
		}
		
		{
		    std::lock_guard<decltype(audio_in_mutex_)> lock(audio_in_mutex_);
		    audio_in_buffer_.push_back(std::make_pair(input_port_i, rx_buffer));
		}
		audio_in_cv_.notify_all();
	    }
	}

	{
	    std::unique_lock<std::mutex> lock(audio_out_mutex_);
	    for (int output_port_i = 0, n = cfg().number_of_modems(); output_port_i < n; ++output_port_i)
	    {	    

		if(output_port_[output_port_i] != nullptr &&
		   !audio_out_buffer_[output_port_i].empty()) 
		{
		    // does the front buffer start within this frame?
		    auto tx_buffer = audio_out_buffer_[output_port_i].front();
		    jack_nframes_t frame_start = 0; // when we begin playback in this frame
		    if(tx_buffer->marker == TaggedAudioBuffer::Marker::START)
		    {		    
			audio_out_index_[output_port_i] = 0;

			if(tx_buffer->buffer->buffer_start_time > buffer_start_time) // playback is in the future, otherwise start asap (0)			   
			    frame_start = (tx_buffer->buffer->buffer_start_time - buffer_start_time)*fs_;
			else
			    frame_start = 0;
			
			if(frame_start > nframes) // this buffer doesn't start yet
			    continue;
			if(audio_out_buffer_[output_port_i].size() < cfg().jack().min_playback_buffer_size())
			{
			    glog.is(DEBUG1) && glog << "Waiting for " << cfg().jack().min_playback_buffer_size() - audio_out_buffer_[output_port_i].size() << " more frames before beginning playback for modem: " << output_port_i << std::endl;
			    continue;
			}
		    }	       

		    sample_t* out = (sample_t*)jack_port_get_buffer (output_port_[output_port_i], nframes);		
		    sample_t* sample = out;
		    for(jack_nframes_t frame = 0; frame < nframes; ++frame)
		    {
			auto& audio_out_index = audio_out_index_[output_port_i];
			if(frame < frame_start)
			{
			    *(sample++) = 0;
			}
			else
			{
			    // while covers the case if the next tx_buffer->size() == 0
			    while(audio_out_index >= tx_buffer->buffer->samples.size())
			    {
				audio_out_index = 0;
				audio_out_buffer_[output_port_i].pop_front();
				if(audio_out_buffer_[output_port_i].empty())
				{
				    // hopefully we only run out when we're at the end of the packet
				    if(tx_buffer->marker != TaggedAudioBuffer::Marker::END)
					glog.is(WARN) && glog << "Missing playback buffer for modem: " << output_port_i << std::endl;
				    tx_buffer = empty_buffer_;
				}
				else
				{
				    tx_buffer = audio_out_buffer_[output_port_i].front();
				}
			    }

			    *(sample++) = tx_buffer->buffer->samples[audio_out_index++];
			}
		    }
		}
	    }
	}
	return 0;
    }

    void jack_shutdown()
    {
	using goby::glog; using namespace goby::common::logger;
	glog.is(DIE) && glog << "Jack has shutdown, so we can't go on." << std::endl;
    }
    int jack_xrun()
    {
	using goby::glog; using namespace goby::common::logger;
	glog.is(WARN) && glog << "Jack buffer xrun (underflow or overflow)" << std::endl;
	return 0;
    }
    int jack_buffer_size_change(jack_nframes_t nframes)
    {
	using goby::glog; using namespace goby::common::logger;
	buffer_size_ = nframes;
	empty_buffer_.reset(new TaggedAudioBuffer);
	empty_buffer_->buffer = std::shared_ptr<AudioBuffer>(new AudioBuffer(buffer_size_));
	glog.is(DEBUG1) && glog << "New buffer size: " << nframes << std::endl;
	interthread().publish<groups::buffer_size_change>(buffer_size_);

	return 0;
    }

    void audio_out(std::shared_ptr<const TaggedAudioBuffer> buffer, int modem_index)
    {
	
	std::unique_lock<std::mutex> lock(audio_out_mutex_);
	if(buffer->marker == TaggedAudioBuffer::Marker::START)
	    audio_out_buffer_[modem_index].clear();
	
	audio_out_buffer_[modem_index].push_back(buffer);
    }
    
    void loop() override
    {
	using goby::glog; using namespace goby::common::logger;
      
	std::pair<int, std::shared_ptr<AudioBuffer>> temp_buffer;
	// grab a element to publish while locked
	{
	    std::unique_lock<std::mutex> lock(audio_in_mutex_);
	    if(audio_in_buffer_.empty()) // if empty, wait until it's not
		audio_in_cv_.wait(lock, [this]{return !audio_in_buffer_.empty();});
	
	    if(audio_in_buffer_.size() >= cfg().jack().max_buffer_size())
	    {
		glog.is(WARN) && glog << "Jack buffer exceeds maximum: " << audio_in_buffer_.size() << std::endl;
		audio_in_buffer_.clear();
		return;
	    }	   
	    temp_buffer = audio_in_buffer_.front();
	}

	// actually publish it while unlocked
	interthread().publish_dynamic(temp_buffer.second, audio_in_groups_[temp_buffer.first]);

	// lock again and pop the front of the buffer
	{
	    std::unique_lock<std::mutex> lock(audio_in_mutex_);
	    audio_in_buffer_.pop_front();
	}       
    }


private:
    std::vector<jack_port_t*> input_port_;
    std::vector<jack_port_t*> output_port_;
    const jack_nframes_t fs_;
    
    std::vector<goby::DynamicGroup> audio_in_groups_;
    std::vector<goby::DynamicGroup> audio_out_groups_;

    jack_client_t *client_;

    const std::string port_type_{std::to_string(sizeof(sample_t)*8) + " bit float mono audio"};

    jack_nframes_t buffer_size_{0};

    std::mutex audio_in_mutex_;
    std::condition_variable audio_in_cv_;
    std::deque<std::pair<int, std::shared_ptr<AudioBuffer>>> audio_in_buffer_;

    std::mutex audio_out_mutex_;
    std::map<int, std::deque<std::shared_ptr<const TaggedAudioBuffer>>> audio_out_buffer_;

    // maps modem index to the current sample within the latest AudioBuffer
    std::map<int, decltype(AudioBuffer::samples)::size_type> audio_out_index_;

    std::shared_ptr<TaggedAudioBuffer> empty_buffer_{0};
};



#endif
