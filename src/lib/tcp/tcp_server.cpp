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

#include "tcp_server.h"

// server
netsim::tcp_server::tcp_server(boost::asio::io_service& io_service, short port)
    : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      socket_(io_service)
{
    accept();
}

void netsim::tcp_server::write(const google::protobuf::Message& message,
                               const boost::asio::ip::tcp::endpoint& ep)
{
    auto it = sessions_.find(ep);
    if (it != sessions_.end())
    {
        if (auto session = it->second.lock())
            session->write(message);
        else
            sessions_.erase(it);
    }
}

void netsim::tcp_server::accept()
{
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
        if (!ec)
        {
            std::unique_ptr<boost::asio::ip::tcp::socket> socket(
                new boost::asio::ip::tcp::socket(std::move(socket_)));
            auto session = std::make_shared<tcp_session>(std::move(socket));
            sessions_.insert(std::make_pair(session->remote_endpoint(), session));

            session->read_callback([this](const std::string& pb_name, const std::string& bytes,
                                          const boost::asio::ip::tcp::endpoint& ep) {
                auto rx_range = rx_callbacks_.equal_range(pb_name);
                for (auto it = rx_range.first; it != rx_range.second; ++it)
                    it->second->post(pb_name, bytes, ep);
            });

            session->start();
        }
        accept();
    });
}
