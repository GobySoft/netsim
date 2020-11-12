#include "processor.h"

void netsim_launch_processor_thread(
    goby::zeromq::MultiThreadApplication<netsim::protobuf::NetSimCoreConfig>* handler,
    int output_index)
{
    switch (output_index)
    {
#define LAUNCH_PROCESSOR(z, n, _) \
    case n: handler->launch_thread<ProcessorThread<n>>(); break;
        BOOST_PP_REPEAT(NETSIM_MAX_MODEMS, LAUNCH_PROCESSOR, nil)
    }
}
