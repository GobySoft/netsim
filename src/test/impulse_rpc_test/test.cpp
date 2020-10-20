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
