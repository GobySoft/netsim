#include "goby/middleware/marshalling/protobuf.h"

#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/middleware/io/udp_one_to_many.h"
#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "messages/groups.h"

#include "lamss/lib_lamss_protobuf/modem_sim.pb.h"

using goby::glog;

constexpr goby::middleware::Group udp_in{"udp_in"};
constexpr goby::middleware::Group udp_out{"udp_out"};

using goby::acomms::protobuf::ModemTransmission;

class NetSimUDP : public goby::zeromq::MultiThreadApplication<NetSimUDPConfig>
{
  public:
    NetSimUDP()
    {
        for (const auto& modem : cfg().modem())
            modems_.insert(std::make_pair(modem.modem_id(), modem));

        launch_thread<goby::middleware::io::UDPOneToManyThread<udp_in, udp_out>>(cfg().udp());

        interthread().subscribe<udp_in, goby::middleware::protobuf::IOData>(
            [this](std::shared_ptr<const goby::middleware::protobuf::IOData> io_msg) {
                ModemTransmission msg;
                msg.ParseFromString(io_msg->data());
                glog.is_debug1() && glog << msg.ShortDebugString() << std::endl;

                if (msg.type() != ModemTransmission::ACK &&
                    (cfg().send_all_as_broadcast() || msg.dest() == goby::acomms::BROADCAST_ID))
                {
                    for (auto& modem_p : modems_)
                    {
                        if (modem_p.first != msg.src())
                            forward_packet(msg.src(), modem_p.first, msg);
                    }
                }
                else if (modems_.count(msg.dest()))
                {
                    forward_packet(msg.src(), msg.dest(), msg);
                }

                else
                {
                    glog.is_warn() && glog << "No modem configured for id: " << msg.dest()
                                           << std::endl;
                }
            });

        interprocess().subscribe<groups::impulse_response, ImpulseResponse>(
            [this](const ImpulseResponse& r) { process_impulse_response(r); });
    }

  private:
    void forward_packet(int src_id, int dest_id, const ModemTransmission& msg);

    void process_impulse_response(const ImpulseReponse& r);
    
  private:
    std::map<int, NetSimUDPConfig::ModemEndPoint> modems_;
};

int main(int argc, char* argv[]) { return goby::run<NetSimUDP>(argc, argv); }

void NetSimUDP::forward_packet(int src_id, int dest_id, const ModemTransmission& msg)
{
    auto msg_out = msg;
    auto& mm_tx = *msg_out.MutableExtension(goby::acomms::micromodem::protobuf::transmission);
    auto owtt = 0.5 * boost::units::si::seconds;

    auto& ranging_reply = *mm_tx.mutable_ranging_reply();
    ranging_reply.add_one_way_travel_time_with_units(owtt);

    auto io_msg_out = std::make_shared<goby::middleware::protobuf::IOData>();
    msg_out.SerializeToString(io_msg_out->mutable_data());
    *io_msg_out->mutable_udp_dest() = modems_.at(dest_id).endpoint();
    interthread().publish<udp_out>(io_msg_out);
}
