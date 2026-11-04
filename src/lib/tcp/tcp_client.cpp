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

#include "tcp_client.h"

std::shared_ptr<netsim::tcp_client> netsim::tcp_client::create(boost::asio::io_context& io_context)
{
    return std::shared_ptr<tcp_client>(new tcp_client(io_context));
}

netsim::tcp_client::tcp_client(boost::asio::io_context& io_context)
    : tcp_session(std::unique_ptr<boost::asio::ip::tcp::socket>(
          new boost::asio::ip::tcp::socket(io_context))),
      io_context_(io_context)
{
}

void netsim::tcp_client::connect(const std::string& server, unsigned short port)
{
    socket().reset(new boost::asio::ip::tcp::socket(io_context_));
    if (io_context_.stopped())
        io_context_.restart();

    boost::asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(server, std::to_string(port));

    boost::asio::async_connect(
        *socket(), endpoints,
        [this](const boost::system::error_code& error, boost::asio::ip::tcp::endpoint)
        {
            if (!error)
                start();
            else
                throw(error);
        });
}
