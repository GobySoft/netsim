# Vehicle Simulator Connections

Each vehicle simulator connects to the netsim system in two ways:

1. The acoustic modem driver connects via TCP to an instance of `socat` which forwards these data to the modem's native interface (serial, TCP, etc.). For example, in the Micro-Modem this is the [serial-based NMEA-0183 interface](https://acomms.whoi.edu/micro-modem/software-interface/). On the *audioserver*, ports 62000-62007 are used for this purpose (for up to 8 modems). This interface is exactly as defined by the modem hardware being used, so no further information is included here.
1. Another process must connect to the `netsim_manager` directly to send updates on the simulated vehicle's position to the *netsim* system. This interface is a simple line-based protocol described below.

## netsim_manager / simulated vehicle interface

### Wire protocol

This interface uses a line-based ASCII protocol with an embedded base-64 encoded Protobuf message:

```
NETSIM|{ASCII protobuf name}|data (base64 encoded version of encoded Protobuf message)\n
```

The request Protobuf message, sent from the vehicle simulator, is always netsim::protobuf::NetSimManagerRequest. The response, sent by `netsim_manager` (the server, by default on port 61999) is always netsim::protobuf::NetSimManagerResponse.

Thus, requests look like:

```
NETSIM|netsim.protobuf.NetSimManagerRequest|CEISOgiz5AMRQ4tUIbbq10EZQUDRT0vLUUAhQylH7tbMYcApMzMzMzMz0z8x65lyHNEpUj85AAAAAAAALkA=\n
```

and responses look like:

```
NETSIM|netsim.protobuf.NetSimManagerResponse|CEEQAQ==\n
```

The easiest way to implement the client side of the protocol (for the vehicle simulator) is to use the `netsim_tcp` library and instantiate a netsim::tcp_client:

```
#include "netsim/tcp/tcp_client.h"

...

boost::asio::io_service io;
std::shared_ptr<netsim::tcp_client> client(netsim::tcp_client::create(io));

std::string netsim_manager_ip = "172.19.21.10";
int netsim_manager_port = 61999;

// connect
client->connect(netsim_manager_ip, netsim_manager_port);

// set a callback to be executed when a response is given
client->read_callback<netsim::protobuf::NetSimManagerResponse>(
        [](const netsim::protobuf::NetSimManagerResponse& t, const boost::asio::ip::tcp::endpoint& ep)
        {
            std::cout << "Received response from [" << ep << "]: " << t.ShortDebugString() << std::endl;
        });

// regularly send a request, including position data
int request_id = 0;
while(1)
{
  // ...fetch vehicle data from the vehicle system...
  
  netsim::protobuf::NetSimManagerRequest req;
  req.set_id(request_id++);
  auto& nav = *req.add_nav();
  nav.set_modem_tcp_port(...); // port used to connect to the modem via socat
  nav.set_time(...); // time in seconds since UNIX
  nav.set_lat(...); // latitude, decimal degrees
  nav.set_lon(...); // longitude, decimal degrees
  nav.set_depth(...); // depth, meters
  nav.set_speed(...); // speed, meters/second
  nav.set_heading(...); // true heading, degrees

  if(client_->connected())
  {
     std::cout << "Sent request (from " << cfg_.node_report_var() << "): " << req.ShortDebugString() << std::endl;
     client->write(req);
  }
}
```

An implementation for the MOOS middleware is given in <https://github.mit.edu/lamss/lamss-shared> as iNetSimManager.

### Request

The request message sent from the vehicle simulator client is:

```
message NetSimManagerRequest
{
    required int32 id = 1;
    repeated NavUpdate nav = 2;
    repeated ReceiveStats stats = 3;
}
```

Each request includes a unique id, a navigation update request (or several: a single client could report position for multiple vehicles), and/or optionally, a receive statistics update push (only implemented for the WHOI Micro-Modem). The `stats` field is used to update the `goby_liaison` display, and has no bearing on the performance of netsim itself.

### Response

```
message NetSimManagerResponse
{
    required int32 request_id = 1;
    enum Status
    {
        UPDATE_ACCEPTED = 1;
        UPDATE_FAILED_INVALID_MODEM_TCP_PORT = 2;
        UPDATE_FAILED_INVALID_SOURCE_ADDRESS = 3;
        UPDATE_FAILED_OUT_OF_DEFINED_REGION = 4;
    }
    required Status status = 2;
}
```

The response includes the same id as the request provided, and a status code for the result of the navigation position update request.

These status values mean:

- UPDATE_ACCEPTED: position update was accepted.
- UPDATE_FAILED_INVALID_MODEM_TCP_PORT: the tcp port used to index this vehicle is not configured in the `netsim_manager` configuration (`sim_env_pair{}`).
- UPDATE_FAILED_INVALID_SOURCE_ADDRESS: the sending IP address is not configured in the `netsim_manager` configuration (`sim_env_pair{}`).
- UPDATE_FAILED_OUT_OF_DEFINED_REGION: the latitude, longitude, or depth given is out of the operation region as defined in the `netsim_manager` configuration (`env_bounds{}` for the environment given in the correspoding `sim_env_pair{}`).

