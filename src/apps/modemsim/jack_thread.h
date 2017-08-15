#ifndef JACKPROCESS20170815H
#define JACKPROCESS20170815H

#include <iostream>
#include <unistd.h>
#include <jack/jack.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "goby/middleware/multi-thread-application.h"

#include "groups.h"
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
    : ThreadBase(config, t)
    {
	static_assert(sizeof(float) == 4, "Float must be 4 bytes");

	const char **ports;
	const char *client_name = "simple";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	
	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);

	if (!client)
	{
	    std::cerr << "jack_client_open() failed, status = " << status << std::endl;
      
	    if (status & JackServerFailed)
	    {
		std::cerr << "Unable to connect to JACK server" << std::endl;
	    }
	    exit(1);
	} 
 
	if (status & JackNameNotUnique)
	{
	    client_name = jack_get_client_name(client);
	    std::cerr << "unique name " << client_name << " assigned." << std::endl;
	}
  
	jack_set_process_callback (client, ::jack_process, this);
	jack_on_shutdown (client, ::jack_shutdown, this);
	jack_set_xrun_callback (client, ::jack_xrun, this);
	jack_set_buffer_size_callback(client, ::jack_buffer_size_change, this);
    
	auto fs_actual = jack_get_sample_rate(client);
	if(fs_actual != fs)
	{
	    std::cerr << "Engine sample rate: " << fs_actual << ", this client requires: " << fs << std::endl;
	    exit(1);
	}
    
	input_port = jack_port_register (client, "input",
					 port_type.c_str(),
					 JackPortIsInput, 0);
	if (!input_port)
	{
	    std::cerr << "no more JACK ports available" << std::endl;
	    exit(1);
	}
  
	if (jack_activate (client))
	{
	    std::cerr << "cannot activate client" << std::endl;
	    exit(1);
	}

	// Connect the ports.  You can't do this before the client is
	// activated, because we can't make connections to clients
	// that aren't running.  Note the confusing (but necessary)
	// orientation of the driver backend ports: playback ports are
	// "input" to the backend, and capture ports are "output" from
	// it.
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL)
	{
	    std::cerr << "no physical capture ports" << std::endl;
	    exit(1);
	}

	int i = 0;
	while(ports[i])
	    std::cerr << "Capture port: " << ports[i++] << std::endl;
	
	if (jack_connect (client, ports[0], jack_port_name (input_port)))
	{
	    std::cerr << "cannot connect input ports" << std::endl;
	    exit(1);
	}

	free (ports);       
    }
  
    ~JackThread()
    {
	jack_client_close (client);
    }

    // The process callback for this JACK application is called in a
    // special realtime thread once for each audio cycle.
    int jack_process (jack_nframes_t nframes)
    {
	using BufferType = std::vector<std::pair<jack_nframes_t, sample_t>>;
	std::shared_ptr<BufferType> rx_buffer(new BufferType(buffer_size));

	sample_t *in;

	auto process_frame_time = jack_last_frame_time(client);
	in = (sample_t*)jack_port_get_buffer (input_port, nframes);

	sample_t* sample = in;

	for(jack_nframes_t frame = 0; frame < nframes; ++frame)
	{
	    (*rx_buffer)[frame] = std::make_pair(process_frame_time + frame, *sample);
	    ++sample;
	}
	
	transporter().inner().publish_dynamic(rx_buffer, groups::audio_in);
	
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
	buffer_size = nframes;
	glog.is(DEBUG1) && glog << "New buffer size: " << nframes << std::endl;
	return 0;
    }


private:
    jack_port_t *input_port;
    jack_client_t *client;

    const std::string port_type{std::to_string(sizeof(sample_t)*8) + " bit float mono audio"};
    const jack_nframes_t fs{96000};

    const sample_t detection_threshold{0.1};
    const unsigned buffer_seconds{10};
    jack_nframes_t buffer_size{0};
    const jack_nframes_t quiet_frames_after_packet{fs/5}; // 0.2 sec of silence = end of packet

    std::mutex rx_buffer_mutex;
    std::condition_variable rx_buffer_cv;
    bool data_ready{false};

};



#endif
