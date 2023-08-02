// Copyright 2017-2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the NETSIM Binaries.
//
// The NETSIM Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The NETSIM Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "bridge.h"
#include "detector.h"
#include "jack_thread.h"
#include "logger.h"
#include "netsim/core/processor.h"
#include "netsim/messages/core_config.pb.h"
#include "netsim/messages/groups.h"

using namespace goby::util::logger;
using goby::glog;

std::atomic<int> bridge_ready{0};
std::atomic<int> detector_ready{0};
std::atomic<int> netsim::processor_ready{0};

class NetSimCore : public goby::zeromq::MultiThreadApplication<netsim::protobuf::NetSimCoreConfig>
{
  public:
    NetSimCore()
    {
        if (cfg().number_of_modems() > NETSIM_MAX_MODEMS)
            glog.is(DIE) && glog << "NETSIM_MAX_MODEMS set to " << NETSIM_MAX_MODEMS
                                 << " at compile time. Increase to use more modems" << std::endl;

        typedef void (*processor_load_func)(
            goby::zeromq::MultiThreadApplication<netsim::protobuf::NetSimCoreConfig> * handler,
            int output_index);
        processor_load_func processor_load_ptr = (processor_load_func)dlsym(
            NetSimCore::processor_library_handle_, "netsim_launch_processor_thread");

        if (!processor_load_ptr)
        {
            glog.is_die() && glog << "Function frontseat_processor_load in library defined "
                                     "in NETSIM_PROCESSOR_LIBRARY does not exist."
                                  << std::endl;
            exit(EXIT_FAILURE);
        }


        int first_modem_index = 0;
        int local_number_of_modems = cfg().number_of_modems();
        if (cfg().has_bridge())
        {
            first_modem_index = cfg().bridge().first_modem_index();
            local_number_of_modems = cfg().bridge().local_number_of_modems();
            launch_thread<BridgeThread>();
            while (!bridge_ready) usleep(10000);
        }

        for (int i = first_modem_index, n = local_number_of_modems + first_modem_index; i < n; ++i)
        {
            // each thread processes the traffic to a given output modem
            (*processor_load_ptr)(this, i);
        }

        while (netsim::processor_ready < local_number_of_modems) usleep(10000);

        switch (local_number_of_modems + first_modem_index)
        {
#define LAUNCH_DETECTOR_THREAD(z, n, _)                     \
    case NETSIM_MAX_MODEMS - n:                             \
        if (NETSIM_MAX_MODEMS - 1 - n >= first_modem_index) \
            launch_thread<DetectorThread<NETSIM_MAX_MODEMS - 1 - n>>();
            BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, LAUNCH_DETECTOR_THREAD, nil)
            break;

            default: break;
        }

        while (detector_ready < local_number_of_modems) usleep(10000);

        switch (local_number_of_modems + first_modem_index)
        {
#define LAUNCH_JACK_THREAD(z, n, _)                         \
    case NETSIM_MAX_MODEMS - n:                             \
        if (NETSIM_MAX_MODEMS - 1 - n >= first_modem_index) \
            launch_thread<JackThread<NETSIM_MAX_MODEMS - 1 - n>>();
            BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, LAUNCH_JACK_THREAD, nil)
            break;
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
