#include <random>

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/middleware/io/udp_one_to_many.h"
#include "goby/zeromq/application/multi_thread.h"
#include "goby/util/sci.h"

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
        : goby::zeromq::MultiThreadApplication<NetSimUDPConfig>(100 * boost::units::si::hertz),
          outlier_occurence_(cfg().outlier_probability()),
          outlier_dowtt_(cfg().outlier_mean(), cfg().outlier_stdev())
    {
        for (const auto& modem : cfg().modem())
        {
            modems_.insert(std::make_pair(modem.modem_id(), modem));
            endpoint_to_id_.insert(
                std::make_pair(modem.endpoint().SerializeAsString(), modem.modem_id()));
            tcp_port_to_id_.insert(
                std::make_pair(std::to_string(modem.modem_tcp_port()), modem.modem_id()));
        }

        for (const auto& range_prob : cfg().range_to_packet_success())
            range_to_packet_success_prob_.insert(
                std::make_pair(range_prob.range(), range_prob.success_probability()));

        if (range_to_packet_success_prob_.size() < 2)
        {
            glog.is_verbose() && glog << "Use 100% packet success. To define range based success "
                                         "rate, use at least 'range_to_packet_success' values"
                                      << std::endl;
            range_to_packet_success_prob_.clear();
        }

        launch_thread<goby::middleware::io::UDPOneToManyThread<udp_in, udp_out>>(cfg().udp());

        interthread().subscribe<udp_in, goby::middleware::protobuf::IOData>(
            [this](std::shared_ptr<const goby::middleware::protobuf::IOData> io_msg) {
                ModemTransmission msg;
                msg.ParseFromString(io_msg->data());
                glog.is_debug1() && glog << msg.ShortDebugString() << std::endl;

                auto src_it = endpoint_to_id_.find(io_msg->udp_src().SerializeAsString());
                if (src_it == endpoint_to_id_.end())
                {
                    glog.is_warn() && glog << "Could not find id for endpoint: "
                                           << io_msg->udp_src().ShortDebugString() << std::endl;
                    return;
                }

                int src_id = src_it->second;

                if (msg.type() != ModemTransmission::ACK &&
                    (cfg().send_all_as_broadcast() || msg.dest() == goby::acomms::BROADCAST_ID))
                {
                    for (auto& modem_p : modems_)
                    {
                        if (modem_p.first != src_id)
                            forward_packet(src_id, modem_p.first, msg);
                    }
                }
                else if (modems_.count(msg.dest()))
                {
                    forward_packet(src_id, msg.dest(), msg);
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

    void process_impulse_response(const ImpulseResponse& r);

    void loop() override;

  private:
    std::map<int, NetSimUDPConfig::ModemEndPoint> modems_;
    std::map<std::string, int> endpoint_to_id_;
    std::map<std::string, int> tcp_port_to_id_;

    // messages waiting for an ImpulseResponse
    // src id -> dest id -> message
    std::map<int, std::multimap<int, ModemTransmission>> forward_buffer_;

    // valid messages waiting for propagation time to elapse
    std::multimap<goby::time::MicroTime, std::shared_ptr<goby::middleware::protobuf::IOData>>
        delay_buffer_;

    // map of range to probability of packet success
    std::map<double, double> range_to_packet_success_prob_;

    std::random_device rd_;
    std::mt19937 rand_gen_{rd_()};
    std::mt19937 outlier_gen_{rd_()};
    // coin flip if a given range is an outlier
    std::bernoulli_distribution outlier_occurence_;
    // what is the actual outlier owtt (added to actual travel time)
    std::normal_distribution<> outlier_dowtt_;
};

int main(int argc, char* argv[]) { return goby::run<NetSimUDP>(argc, argv); }

void NetSimUDP::forward_packet(int src_id, int dest_id, const ModemTransmission& msg)
{
    static int imp_req_id(0);
    ImpulseRequest imp_req;
    imp_req.set_request_time(goby::time::SystemClock::now<goby::time::SITime>().value());
    imp_req.set_request_id(imp_req_id++);
    imp_req.set_source(std::to_string(modems_.at(src_id).modem_tcp_port()));
    imp_req.set_receiver(std::to_string(modems_.at(dest_id).modem_tcp_port()));
    interprocess().publish<groups::impulse_request>(imp_req);
    goby::glog.is_debug1() && goby::glog << "Sent impulse request: " << imp_req.ShortDebugString()
                                         << std::endl;

    forward_buffer_[src_id].insert(std::make_pair(dest_id, msg));
}

void NetSimUDP::loop()
{
    auto now = goby::time::SystemClock::now<goby::time::MicroTime>();

    auto upper_it = delay_buffer_.upper_bound(now);

    for (auto it = delay_buffer_.begin(); it != upper_it; ++it)
        interthread().publish<udp_out>(it->second);

    delay_buffer_.erase(delay_buffer_.begin(), upper_it);
}

void NetSimUDP::process_impulse_response(const ImpulseResponse& r)
{
    int src_id = tcp_port_to_id_.at(r.source());
    int dest_id = tcp_port_to_id_.at(r.receiver());

    auto it_p = forward_buffer_[src_id].equal_range(dest_id);

    if (r.raytrace_size())
    {
        double first_arrival = std::numeric_limits<double>::infinity();
        double outlier_time = 0;
        for (const auto& ray : r.raytrace())
        {
            // randomly add an offset to the owtt for simulating outliers
            if (outlier_occurence_(outlier_gen_))
                outlier_time = outlier_dowtt_(outlier_gen_);

            if (ray.element_size() > 0 && ray.element(0).delay() < first_arrival)
                first_arrival = ray.element(0).delay();
        }

        goby::glog.is_debug1() && goby::glog << "Received impulse response: " << r.source() << "->"
                                             << r.receiver() << ", first arrival: " << first_arrival
                                             << ", outlier dt: " << outlier_time << std::endl;

        for (auto it = it_p.first; it != it_p.second; ++it)
        {
            auto msg = it->second;
            auto& mm_tx = *msg.MutableExtension(goby::acomms::micromodem::protobuf::transmission);
            auto owtt = (first_arrival + outlier_time) * boost::units::si::seconds;

            auto approx_range = owtt * cfg().sound_speed_with_units();

            bool packet_success = true;

            if(!range_to_packet_success_prob_.empty())
            {
                double p = goby::util::linear_interpolate<double, double>(approx_range/boost::units::si::meters, range_to_packet_success_prob_);

                glog.is_debug1() && glog << "Probability of success: " << p << std::endl;
                std::bernoulli_distribution d(p);
                packet_success = d(rand_gen_);
            }
            
            if (packet_success)
            {
                auto& ranging_reply = *mm_tx.mutable_ranging_reply();
                ranging_reply.add_one_way_travel_time_with_units(owtt);
                auto io_msg_out = std::make_shared<goby::middleware::protobuf::IOData>();
                msg.SerializeToString(io_msg_out->mutable_data());
                *io_msg_out->mutable_udp_dest() = modems_.at(dest_id).endpoint();

                auto resend_time = msg.time_with_units() + goby::time::MicroTime(owtt);
                delay_buffer_.insert(std::make_pair(resend_time, io_msg_out));

                goby::glog.is_debug1() && goby::glog << "Delaying " << src_id << "->" << dest_id
                                                     << " message until: " << resend_time.value()
                                                     << std::endl;
            }
            else
            {
                goby::glog.is_debug1() && goby::glog << "Dropping " << src_id << "->" << dest_id
                                                     << " message (failed coin flip)" << std::endl;
            }
        }
    }
    else
    {
        goby::glog.is_warn() && goby::glog << "Received empty impulse response: " << r.source()
                                           << "->" << r.receiver() << ", discarding "
                                           << std::distance(it_p.first, it_p.second) << " messages"
                                           << std::endl;
    }

    forward_buffer_[src_id].erase(it_p.first, it_p.second);
}
