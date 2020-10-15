#ifndef TLPLOT20180821H
#define TLPLOT20180821H

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "netsim/acousticstoolbox/iBellhop_messages.pb.h"

using ThreadBase = goby::middleware::SimpleThread<NetSimPostprocessConfig>;

class TLPlotThread : public ThreadBase
{
public:

TLPlotThread(const NetSimPostprocessConfig& config)
    : ThreadBase(config)
    {
	using goby::glog;

        interprocess().subscribe<netsim::groups::bellhop_response,
            iBellhopResponse>(
                [this](const iBellhopResponse& resp)
                {
		    if(resp.requestor().find("goby::moos::Translator") != std::string::npos)
		    {
			if(resp.success())
			{
			    
			    std::stringstream octave_cmd;
			    octave_cmd << "flatpak run org.octave.Octave /opt/netsim/src/octave/liaison_create_shd.m " << resp.output_file();
			    glog.is_debug1() && glog << "Running: " << octave_cmd.str() << std::endl;
			    system(octave_cmd.str().c_str());
			    glog.is_debug1() && glog << "Octave complete" << std::endl;
			}
			
			interprocess().publish<netsim::groups::post_process_event>(resp);
		    }
                });
    }

private:

    
};

#endif
