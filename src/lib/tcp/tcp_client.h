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

#ifndef TCP_CLIENT_20180131H
#define TCP_CLIENT_20180131H

#include "tcp_session.h"

namespace netsim
{
class tcp_client : public tcp_session
{
  public:
    static std::shared_ptr<tcp_client> create(boost::asio::io_service& io_service);

    void connect(const std::string& server, unsigned short port);

  private:
    tcp_client(boost::asio::io_service& io_service);

  private:
    boost::asio::io_service& io_service_;
};

} // namespace netsim

#endif
