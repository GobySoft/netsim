#include <boost/algorithm/string.hpp>
#include <iostream>

#include "tcp_session.h"

// session
netsim::tcp_session::tcp_session(std::unique_ptr<boost::asio::ip::tcp::socket> socket)
    : socket_(std::move(socket))
{
}

void netsim::tcp_session::start()
{
    self_ = shared_from_this();
    read();
}

void netsim::tcp_session::read()
{
    async_read_until(*socket_, buffer_, end_of_line_,
                     [this](boost::system::error_code ec, std::size_t length) {
                         if (!ec)
                         {
                             std::string preamble, pb_name, pb_data, pb_data_b64;
                             std::istream is(&buffer_);
                             std::getline(is, preamble, delimiter_);
                             std::getline(is, pb_name, delimiter_);
                             std::getline(is, pb_data_b64, end_of_line_);
                             pb_data = dccl::b64_decode(pb_data_b64);
                             auto rx_range = rx_callbacks_.equal_range(pb_name);

                             boost::asio::ip::tcp::endpoint remote;
                             // the remote may no longer be connected
                             try
                             {
                                 remote = socket_->remote_endpoint();
                             }
                             catch (boost::exception& e)
                             {
                             }

                             for (auto it = rx_range.first; it != rx_range.second; ++it)
                                 it->second->post(pb_name, pb_data, remote);
                             for (auto& rx : rx_all_) rx->post(pb_name, pb_data, remote);

                             read();
                         }
                         else
                         {
                             disconnect();
                         }
                     });
}

void netsim::tcp_session::write(const google::protobuf::Message& message)
{
    std::string bytes = preamble_ + delimiter_ + message.GetDescriptor()->full_name() + delimiter_ +
                        boost::erase_all_copy(dccl::b64_encode(message.SerializeAsString()), "\n") +
                        end_of_line_;

    boost::asio::async_write(*socket_, boost::asio::buffer(bytes),
                             [this](boost::system::error_code ec, std::size_t length) {
                                 if (ec)
                                 {
                                     disconnect();
                                 }
                             });
}

void netsim::tcp_session::disconnect() { self_.reset(); }
