#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/single_thread.h"

#include "netsim/messages/groups.h"
#include "config.pb.h"

#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"


class ImpulseRPCTest : public goby::zeromq::SingleThreadApplication<ImpulseRPCTestConfig>
{
public:
    ImpulseRPCTest() : goby::zeromq::SingleThreadApplication<ImpulseRPCTestConfig>(0.1*boost::units::si::hertz)
        {
            interprocess().subscribe<groups::impulse_response, ImpulseResponse>(
                [](const ImpulseResponse& imp_res)
                {
                    std::cout << "IMPULSE_RESPONSE: " <<  imp_res.DebugString() << std::endl;
                }
                );
        }

private:
    void loop() override
        {
            ImpulseRequest imp_req;
            imp_req.set_source("macrura");
            imp_req.set_receiver("shelfbreak");
            interprocess().publish<groups::impulse_request>(imp_req);
            std::cout << "publishing: " << imp_req.ShortDebugString() << std::endl;
        }    
};

    
int main(int argc, char* argv[])
{ return goby::run<ImpulseRPCTest>(argc, argv); }
