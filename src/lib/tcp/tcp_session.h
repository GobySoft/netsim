// Copyright 2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the NETSIM Libraries.
//
// The NETSIM Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The NETSIM Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#ifndef TCP_SESSION_20180131H
#define TCP_SESSION_20180131H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/asio.hpp>

#include <google/protobuf/message.h>

#include <dccl/binary.h>

#include <goby/util/asio_compat.h>

namespace netsim
{
class ReceiveBase
{
  public:
    ReceiveBase() = default;
    virtual ~ReceiveBase() = default;

    virtual void post(const std::string& pb_name, const std::string& bytes,
                      const boost::asio::ip::tcp::endpoint& remote) = 0;
};

template <typename ProtobufMessage> class Receive : public ReceiveBase
{
  public:
    using CallbackType = std::function<void(const ProtobufMessage& msg,
                                            const boost::asio::ip::tcp::endpoint& remote)>;

    Receive(CallbackType f) : f_(f) {}
    void post(const std::string& pb_name, const std::string& bytes,
              const boost::asio::ip::tcp::endpoint& remote)
    {
        ProtobufMessage msg;
        msg.ParseFromString(bytes);
        f_(msg, remote);
    }

  private:
    CallbackType f_;
};

class ReceiveUnparsed : public ReceiveBase
{
  public:
    using CallbackType = std::function<void(const std::string& pb_name, const std::string& bytes,
                                            const boost::asio::ip::tcp::endpoint& remote)>;
    ReceiveUnparsed(CallbackType f) : f_(f) {}
    void post(const std::string& pb_name, const std::string& bytes,
              const boost::asio::ip::tcp::endpoint& remote)
    {
        f_(pb_name, bytes, remote);
    }

  private:
    CallbackType f_;
};

class tcp_session : public std::enable_shared_from_this<tcp_session>
{
  public:
    tcp_session(std::unique_ptr<boost::asio::ip::tcp::socket> socket);
    void start();

    // receive all messages (unparsed)
    void read_callback(typename ReceiveUnparsed::CallbackType f)
    {
        std::unique_ptr<ReceiveBase> rx(new ReceiveUnparsed(f));
        rx_all_.insert(std::move(rx));
    }

    // receive specific types (parsed)
    template <typename ProtobufMessage>
    void read_callback(typename Receive<ProtobufMessage>::CallbackType f)
    {
        std::unique_ptr<ReceiveBase> rx(new Receive<ProtobufMessage>(f));
        rx_callbacks_.insert(
            std::make_pair(ProtobufMessage::descriptor()->full_name(), std::move(rx)));
    }

    void write(const google::protobuf::Message& message);
    void disconnect();
    bool connected() { return bool(self_); };

    boost::asio::ip::tcp::endpoint remote_endpoint()
    {
        if (socket_)
            return socket_->remote_endpoint();
        else
            return boost::asio::ip::tcp::endpoint();
    }

  protected:
    std::unique_ptr<boost::asio::ip::tcp::socket>& socket() { return socket_; }

    // NETSIM|[ASCII protobuf name, e.g. goby.moos.protobuf.NodeStatus]|data (b64)\n
  private:
    void read();
    void name_read();
    void data_read(std::uint32_t size);

  private:
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    // used to indicate that we are still valid (connected)
    // if we reset this, the server should clean up this connection
    std::shared_ptr<netsim::tcp_session> self_;

    // protobuf full name -> Receive
    std::unordered_multimap<std::string, std::unique_ptr<ReceiveBase>> rx_callbacks_;
    std::set<std::unique_ptr<ReceiveBase>> rx_all_;

    boost::asio::streambuf buffer_;

    const char end_of_line_{'\n'};
    const char delimiter_{'|'};
    const std::string preamble_{"NETSIM"};
};
} // namespace netsim

#endif
