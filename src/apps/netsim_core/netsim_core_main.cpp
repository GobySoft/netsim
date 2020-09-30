#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "netsim/messages/core_config.pb.h"
#include "detector.h"
#include "jack_thread.h"
#include "logger.h"
#include "netsim/messages/groups.h"
#include "netsim/core/processor.h"

using namespace goby::util::logger;
using goby::glog;

class NetSimCore : public goby::zeromq::MultiThreadApplication<NetSimCoreConfig>
{
  public:
    NetSimCore()
    {
        typedef void (*processor_load_func)(goby::zeromq::MultiThreadApplication<NetSimCoreConfig>* handler, int output_index);
        processor_load_func processor_load_ptr = (processor_load_func)dlsym(
            NetSimCore::processor_library_handle_, "netsim_launch_processor_thread");

        if (!processor_load_ptr)
        {
            glog.is_die() && glog << "Function frontseat_processor_load in library defined "
                                     "in NETSIM_PROCESSOR_LIBRARY does not exist."
                                  << std::endl;
            exit(EXIT_FAILURE);
        }

        if (cfg().node_name_size() != cfg().number_of_modems())
            glog.is(DIE) && glog << "The node_name field must be specified number_of_modem times"
                                 << std::endl;

        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            // each thread processes the traffic to a given output modem
            (*processor_load_ptr)(this, i);
        }

        while (ProcessorThreadBase::ready < cfg().number_of_modems()) usleep(10000);

        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            // each thread handles the traffic from a given modem
            launch_thread<DetectorThread>(i);
        }

        while (DetectorThread::ready < cfg().number_of_modems()) usleep(10000);

        for (int i = 0, n = cfg().number_of_modems(); i < n; ++i)
        {
            // each thread handles all traffic originating from a given modem
            // and to all dest modems
            launch_thread<JackThread>(i);
        }

        if (cfg().logger().run_logger())
            launch_thread<LoggerThread>();
    }

    static void* processor_library_handle_;
};

void* NetSimCore::processor_library_handle_ = 0;

int main(int argc, char* argv[])
{
    // load plugin processor from environmental variable NETSIM_PROCESSOR_LIBRARY
    char* processor_lib_path = getenv("NETSIM_PROCESSOR_LIBRARY");
    if (processor_lib_path)
    {
        std::cerr << "Loading processor library: " << processor_lib_path << std::endl;
        NetSimCore::processor_library_handle_ = dlopen(processor_lib_path, RTLD_LAZY);
        if (!NetSimCore::processor_library_handle_)
        {
            std::cerr << "Failed to open library: " << processor_lib_path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        std::cerr << "Environmental variable NETSIM_PROCESSOR_LIBRARY must be set with name of "
                     "the dynamic library containing the specific processor to use."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    return goby::run<NetSimCore>(argc, argv);
}
