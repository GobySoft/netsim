#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "messages/groups.h"

using namespace goby::common::logger;
using goby::glog;

class NetSimTool : public goby::MultiThreadApplication<NetSimToolConfig>
{
public:
    NetSimTool()
        {
	}
};



int main(int argc, char* argv[])
{ return goby::run<NetSimTool>(argc, argv); }
