#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "messages/groups.h"
#include "jack_thread.h"
#include "detector.h"
#include "processor.h"
#include "logger.h"

using namespace goby::util::logger;
using goby::glog;

class NetSimCore : public goby::zeromq::MultiThreadApplication<NetSimCoreConfig>
{
public:
    NetSimCore()
        {
            if(cfg().node_name_size() != cfg().number_of_modems())
                glog.is(DIE) && glog << "The node_name field must be specified number_of_modem times" << std::endl;
            
	    for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	    {
		// each thread processes the traffic to a given output modem
		launch_thread<ProcessorThread>(i);
	    }

	    while(ProcessorThread::ready < cfg().number_of_modems())
		usleep(10000);
	    
	    for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	    {
		// each thread handles the traffic from a given modem
		launch_thread<DetectorThread>(i);
	    }

	    while(DetectorThread::ready < cfg().number_of_modems())
		usleep(10000);
	    
	    for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	    {
		// each thread handles all traffic originating from a given modem
		// and to all dest modems
		launch_thread<JackThread>(i);
	    }
	    
	    if(cfg().logger().run_logger())
		launch_thread<LoggerThread>(); 
	}
};



int main(int argc, char* argv[])
{ return goby::run<NetSimCore>(argc, argv); }
