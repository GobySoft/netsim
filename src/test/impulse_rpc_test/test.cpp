#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/single_thread.h"

#include "netsim/messages/groups.h"
#include "config.pb.h"

#include "netsim/messages/netsim.pb.h"


class ImpulseRPCTest : public goby::zeromq::SingleThreadApplication<ImpulseRPCTestConfig>
{
public:
    ImpulseRPCTest() : goby::zeromq::SingleThreadApplication<ImpulseRPCTestConfig>(0.1*boost::units::si::hertz)
        {
            interprocess().subscribe<netsim::groups::impulse_response, netsim::protobuf::ImpulseResponse>(
                [](const netsim::protobuf::ImpulseResponse& imp_res)
                {
                    std::cout << "IMPULSE_RESPONSE: " <<  imp_res.DebugString() << std::endl;
                }
                );
        }

private:
    void loop() override
        {
            netsim::protobuf::ImpulseRequest imp_req;
            imp_req.set_source("macrura");
            imp_req.set_receiver("shelfbreak");
            interprocess().publish<netsim::groups::impulse_request>(imp_req);
            std::cout << "publishing: " << imp_req.ShortDebugString() << std::endl;
        }    
};

    
int main(int argc, char* argv[])
{ return goby::run<ImpulseRPCTest>(argc, argv); }
