add_executable(netsim_manager
  manager.cpp)

target_link_libraries(netsim_manager goby goby_zeromq ${LAMSS_PROTOBUF_LIBRARY} netsim_tcp netsim_messages)
