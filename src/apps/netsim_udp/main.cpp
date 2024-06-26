// Copyright 2019-2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Henrik Schmidt <henrik@mit.edu>
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

#include <random>

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/middleware/io/udp_one_to_many.h"
#include "goby/util/sci.h"
#include "goby/zeromq/application/multi_thread.h"

#include "config.pb.h"
#include "netsim/messages/groups.h"

#include "netsim/messages/netsim.pb.h"

using goby::glog;

constexpr goby::middleware::Group udp_in{"udp_in"};
constexpr goby::middleware::Group udp_out{"udp_out"};

using goby::acomms::protobuf::ModemTransmission;

class ModemPairPerformance
{
  public:
    int src_id;
    int dest_id;
    double mpp;
    double mpp_probab();
};

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

        if (cfg().packet_success_method() == NetSimUDPConfig::RANGE_TABLE &&
            range_to_packet_success_prob_.size() < 2)
        {
            glog.is_die() && glog << "To use RANGE_TABLE packet success method, you must use at "
                                     "least two 'range_to_packet_success' values"
                                  << std::endl;
        }

        if (cfg().packet_success_method() == NetSimUDPConfig::MPP)
        {
            // Create Performance table for modem combinations
            perf_count_ = 0;
            perf_table_.clear();
            ModemPairPerformance new_perf;
            for (int i = 0; i < cfg().modem_size(); i++)
            {
                new_perf.src_id = cfg().modem(i).modem_id();
                for (int j = 0; j < i; j++)
                {
                    new_perf.dest_id = cfg().modem(j).modem_id();
                    new_perf.mpp = 80.0; // prob = 1.
                    perf_table_.push_back(new_perf);
                }
            }
        }

        launch_thread<goby::middleware::io::UDPOneToManyThread<udp_in, udp_out>>(cfg().udp());

        interthread().subscribe<udp_in, goby::middleware::protobuf::IOData>(
            [this](std::shared_ptr<const goby::middleware::protobuf::IOData> io_msg)
            {
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

        if (cfg().packet_success_method() == NetSimUDPConfig::MPP)
        {
            interprocess()
                .subscribe<netsim::groups::performance_response, netsim::protobuf::ObjFuncResponse>(
                    [this](const netsim::protobuf::ObjFuncResponse& r)
                    { process_performance_response(r); });
        }

        interprocess()
            .subscribe<netsim::groups::impulse_response, netsim::protobuf::ImpulseResponse>(
                [this](const netsim::protobuf::ImpulseResponse& r)
                { process_impulse_response(r); });
    }

  private:
    void forward_packet(int src_id, int dest_id, const ModemTransmission& msg);
    void performance_request(int src_id, int dest_id);

    void process_impulse_response(const netsim::protobuf::ImpulseResponse& r);
    void process_performance_response(const netsim::protobuf::ObjFuncResponse& r);
    double mpp_transmit_probability(int src_id, int dest_id);

    void loop() override;
    double travel_time(netsim::protobuf::ImpulseResponse impulse_response);

  private:
    std::map<int, NetSimUDPConfig::ModemEndPoint> modems_;
    std::map<std::string, int> endpoint_to_id_;
    std::map<std::string, int> tcp_port_to_id_;

    std::vector<ModemPairPerformance> perf_table_;
    int perf_count_;
    // messages waiting for an netsim::protobuf::ImpulseResponse
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
    if (cfg().packet_success_method() == NetSimUDPConfig::MPP)
    {
        // performance_request
        performance_request(src_id, dest_id);
    }

    static int imp_req_id(0);
    netsim::protobuf::ImpulseRequest imp_req;
    imp_req.set_request_time(goby::time::SystemClock::now<goby::time::SITime>().value());
    imp_req.set_request_id(imp_req_id++);
    imp_req.set_source(std::to_string(modems_.at(src_id).modem_tcp_port()));
    imp_req.set_receiver(std::to_string(modems_.at(dest_id).modem_tcp_port()));
    interprocess().publish<netsim::groups::impulse_request>(imp_req);
    goby::glog.is_debug1() && goby::glog << "Sent impulse request: " << imp_req.ShortDebugString()
                                         << std::endl;

    forward_buffer_[src_id].insert(std::make_pair(dest_id, msg));
}

void NetSimUDP::performance_request(int src_id, int dest_id)
{
    static int perf_req_id(0);
    netsim::protobuf::ObjFuncRequest perf_req;
    perf_req.set_request_time(goby::time::SystemClock::now<goby::time::SITime>().value());
    perf_req.set_request_id(perf_req_id++);
    perf_req.set_requestor("netsim_udp");
    perf_req.set_contact(std::to_string(modems_.at(src_id).modem_tcp_port()));
    netsim::protobuf::ObjFuncRequest::Receiver* receiver = 0;
    receiver = perf_req.add_receiver();
    receiver->set_node(std::to_string(modems_.at(dest_id).modem_tcp_port()));
    interprocess().publish<netsim::groups::performance_request>(perf_req);
    goby::glog.is_debug1() &&
        goby::glog << "Sent performance request: " << perf_req.ShortDebugString() << std::endl;
}

void NetSimUDP::loop()
{
    auto now = goby::time::SystemClock::now<goby::time::MicroTime>();
    auto upper_it = delay_buffer_.upper_bound(now);

    for (auto it = delay_buffer_.begin(); it != upper_it; ++it)
        interthread().publish<udp_out>(it->second);

    delay_buffer_.erase(delay_buffer_.begin(), upper_it);
}

void NetSimUDP::process_performance_response(const netsim::protobuf::ObjFuncResponse& r)
{
    //  if (r.requestor() != "netsim_udp" )
    //    return;

    goby::glog.is_debug1() &&
        goby::glog << "Received performance response: " << r.ShortDebugString() << std::endl;

    if (!tcp_port_to_id_.count(r.contact()))
    {
        glog.is_debug1() && glog << "Ignoring process_performance_response for unknown contact"
                                 << std::endl;
        return;
    }

    int src_id = tcp_port_to_id_.at(r.contact());

    for (int i = 0; i < r.receiver_size(); i++)
    {
        int dest_id = tcp_port_to_id_.at(r.receiver(i).node());
        // insert mpp in table
        for (int j = 0; j < perf_table_.size(); j++)
        {
            if ((src_id == perf_table_[j].src_id && dest_id == perf_table_[j].dest_id) ||
                (dest_id == perf_table_[j].src_id && src_id == perf_table_[j].dest_id))
            {
                perf_table_[j].mpp = r.receiver(i).mpp();
                glog.is_debug1() &&
                    glog << "MPP performance received for src_id=" << src_id
                         << ", dest_id=" << dest_id << ", mpp=" << perf_table_[j].mpp
                         << ", mpp_probab=" << perf_table_[j].mpp_probab() << std::endl;
            }
        }
    }
}

double NetSimUDP::mpp_transmit_probability(int src_id, int dest_id)
{
    for (int j = 0; j < perf_table_.size(); j++)
        if ((src_id == perf_table_[j].src_id && dest_id == perf_table_[j].dest_id) ||
            (dest_id == perf_table_[j].src_id && src_id == perf_table_[j].dest_id))
        {
            return (perf_table_[j].mpp_probab());
        }
    return 0;
}

void NetSimUDP::process_impulse_response(const netsim::protobuf::ImpulseResponse& r)
{
    if (!tcp_port_to_id_.count(r.source()) || !tcp_port_to_id_.at(r.receiver()))
    {
        glog.is_debug1() && glog << "Ignoring impulse response for unknown source/receiver pair"
                                 << std::endl;
        return;
    }

    int src_id = tcp_port_to_id_.at(r.source());
    int dest_id = tcp_port_to_id_.at(r.receiver());

    auto it_p = forward_buffer_[src_id].equal_range(dest_id);

    if (r.raytrace_size())
    {
        double first_arrival = travel_time(r);
        double outlier_time = 0;
        // randomly add an offset to the owtt for simulating outliers
        if (outlier_occurence_(outlier_gen_))
            outlier_time = outlier_dowtt_(outlier_gen_);

        goby::glog.is_debug1() && goby::glog << "Received impulse response: " << r.source() << "->"
                                             << r.receiver() << ", arrival time: " << first_arrival
                                             << ", outlier dt: " << outlier_time << std::endl;

        for (auto it = it_p.first; it != it_p.second; ++it)
        {
            auto msg = it->second;
            auto& mm_tx = *msg.MutableExtension(goby::acomms::micromodem::protobuf::transmission);
            auto owtt = (first_arrival + outlier_time) * boost::units::si::seconds;

            auto approx_range = owtt * cfg().sound_speed_with_units();

            bool packet_success = true;

            if (cfg().packet_success_method() != NetSimUDPConfig::ALL_PACKETS_SUCCEED)
            {
                double p = 0;
                if (cfg().packet_success_method() == NetSimUDPConfig::RANGE_TABLE)
                {
                    p = goby::util::linear_interpolate<double, double>(
                        approx_range / boost::units::si::meters, range_to_packet_success_prob_);
                }
                else if (cfg().packet_success_method() == NetSimUDPConfig::MPP)
                {
                    p = mpp_transmit_probability(src_id, dest_id);
                }

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

double NetSimUDP::travel_time(netsim::protobuf::ImpulseResponse impulse_response)
{
    // returns delay for strongest direct ray
    double max_amp = 0;
    int max_indx = 0;
    double delay;
    for (int i = 0; i < impulse_response.raytrace_size(); i++)
    {
        double amp = std::fabs(impulse_response.raytrace(i).amplitude());
        int bounces = impulse_response.raytrace(i).surface_bounces() +
                      impulse_response.raytrace(i).bottom_bounces();
        //        if (amp > max_amp && bounces == 0)
        if (amp > max_amp)
        {
            max_amp = amp;
            max_indx = i;
            delay = impulse_response.raytrace(i).element(0).delay();
        }
    }
    glog.is_debug2() && glog << "travel_time: max_indx " << max_indx << std::endl;
    return delay;
}

double ModemPairPerformance::mpp_probab()
{
    return (std::max(0.0, std::min(1.0, ((mpp - 30) * 80. / 50. + 20) / 100.0)));
}
