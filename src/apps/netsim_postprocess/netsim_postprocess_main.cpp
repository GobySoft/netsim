#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "messages/groups.h"
#include "audio_images.h"
#include "tl_plot.h"

using namespace goby::util::logger;
using goby::glog;

class NetSimPostprocess : public goby::zeromq::MultiThreadApplication<NetSimPostprocessConfig>
{
public:
    NetSimPostprocess()
        {
            launch_thread<AudioImagesThread>(); 
            launch_thread<TLPlotThread>(); 
	}
};



int main(int argc, char* argv[])
{ return goby::run<NetSimPostprocess>(argc, argv); }
