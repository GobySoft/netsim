#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "messages/groups.h"
#include "jack_thread.h"
#include "detector.h"
#include "processor.h"
#include "logger.h"

class ModemSim : public goby::MultiThreadApplication<ModemSimConfig>
{
public:
    ModemSim()
        {
            launch_thread<JackThread>();

	    for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
		launch_thread<DetectorThread>(i);
	    for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
		launch_thread<ProcessorThread>(i);            

	    if(cfg().logger().run_logger())
		launch_thread<LoggerThread>(); 
	}
};



int main(int argc, char* argv[])
{ return goby::run<ModemSim>(argc, argv); }
