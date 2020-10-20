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

#ifndef TCP_SERVER_20180131H
#define TCP_SERVER_20180131H

#include <list>

#include "tcp_session.h"

namespace netsim
{
class tcp_server
{
  public:
    tcp_server(boost::asio::io_service& io_service, short port);
    void write(const google::protobuf::Message& message, const boost::asio::ip::tcp::endpoint& ep);

    template <typename ProtobufMessage>
    void read_callback(typename Receive<ProtobufMessage>::CallbackType f)
    {
        std::unique_ptr<ReceiveBase> rx(new Receive<ProtobufMessage>(f));
        rx_callbacks_.insert(
            std::make_pair(ProtobufMessage::descriptor()->full_name(), std::move(rx)));
    }

  private:
    void accept();
    std::map<boost::asio::ip::tcp::endpoint, std::weak_ptr<tcp_session>> sessions_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;

    std::unordered_multimap<std::string, std::unique_ptr<ReceiveBase>> rx_callbacks_;
};
} // namespace netsim

#endif
