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

}

#endif
