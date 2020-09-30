add_library(netsim_tcp SHARED
  tcp_client.cpp
  tcp_server.cpp
  tcp_session.cpp)

target_link_libraries(netsim_tcp ${Boost_LIBRARIES} ${GOBY_LIBRARIES} b64)
