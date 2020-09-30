#include "tcp_client.h"

std::shared_ptr<netsim::tcp_client> netsim::tcp_client::create(boost::asio::io_service& io_service)
{
    return std::shared_ptr<tcp_client>(new tcp_client(io_service));
}


netsim::tcp_client::tcp_client(boost::asio::io_service& io_service) :
    tcp_session(std::unique_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(io_service))),
    io_service_(io_service)
{
}

void netsim::tcp_client::connect(const std::string& server, unsigned short port)
{
    socket().reset(new boost::asio::ip::tcp::socket(io_service_));
    if(io_service_.stopped())
	io_service_.reset();


    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));
    auto endpoints = resolver.resolve(query);
    
    boost::asio::async_connect(*socket(),
                               endpoints,
#ifdef USE_BOOST_IO_SERVICE
                               [this](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator i)
#else
                               [this](const boost::system::error_code& error, boost::asio::ip::tcp::endpoint i)
#endif                              
                               {
                                   if(!error)
                                       start();
                                   else
                                       throw(error);
                               });
    
}
