#ifndef JACKTHREAD20170815H
#define JACKTHREAD20170815H

#include <iostream>
#include <unistd.h>
#include <jack/jack.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "goby/middleware/multi-thread-application.h"
#include "config.pb.h"

using ThreadBase = goby::Thread<ModemSimConfig, goby::InterProcessForwarder<goby::InterThreadTransporter>>;

int jack_process(jack_nframes_t nframes, void* arg);
void jack_shutdown (void *arg);
int jack_xrun(void* arg);
int jack_buffer_size_change(jack_nframes_t nframes, void *arg);




class JackThread : public ThreadBase
{
public:
    typedef float sample_t;

JackThread(const ModemSimConfig& config, ThreadBase::Transporter* t)
    : ThreadBase(config, t),
	input_port_(config.number_of_modems(), nullptr)      
    {
	using goby::glog; using namespace goby::common::logger;
	static_assert(sizeof(float) == 4, "Float must be 4 bytes");

	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    audio_in_groups_.push_back(goby::DynamicGroup(std::string("audio_in_") + std::to_string(i)));
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
	const char** capture_port_names = jack_get_ports (client_, nullptr, nullptr,
							  JackPortIsPhysical|JackPortIsOutput);
	if (capture_port_names == nullptr)
	    glog.is(DIE) && glog <<  "no physical capture ports" << std::endl;	    

	if (jack_activate (client_))
	    glog.is(DIE) && glog << "cannot activate client" << std::endl;	   


	int j = 0;
	while(capture_port_names[j])
	    glog.is(DEBUG2) && glog << "Capture port: " << capture_port_names[j++] << std::endl;

	
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
    }
  
    ~JackThread()
    {
	jack_client_close (client_);
    }

    // The process callback for this JACK application is called in a
    // special realtime thread once for each audio cycle.
    int jack_process (jack_nframes_t nframes)
    {
	using BufferType = std::vector<std::pair<jack_nframes_t, sample_t>>;      

	auto process_frame_time = jack_last_frame_time(client_);

	for (int input_port_i = 0, n = cfg().number_of_modems(); input_port_i < n; ++input_port_i)
	{	    
	    std::shared_ptr<BufferType> rx_buffer(new BufferType(buffer_size_));
	    
	    sample_t* in = (sample_t*)jack_port_get_buffer (input_port_[input_port_i], nframes);
	    sample_t* sample = in;
	    for(jack_nframes_t frame = 0; frame < nframes; ++frame)
	    {
		(*rx_buffer)[frame] = std::make_pair(process_frame_time + frame, *sample);
		++sample;
	    }
	    transporter().inner().publish_dynamic(rx_buffer, audio_in_groups_[input_port_i]);
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
	glog.is(DEBUG1) && glog << "New buffer size: " << nframes << std::endl;
	return 0;
    }


private:
    std::vector<jack_port_t*> input_port_;
    
    std::vector<goby::DynamicGroup> audio_in_groups_;
    jack_client_t *client_;

    const std::string port_type_{std::to_string(sizeof(sample_t)*8) + " bit float mono audio"};
    const jack_nframes_t fs_{96000};

    jack_nframes_t buffer_size_{0};
};



#endif
