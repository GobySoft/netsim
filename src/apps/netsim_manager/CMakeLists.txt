add_executable(netsim_manager
  manager.cpp)

target_link_libraries(netsim_manager goby goby_zeromq ${LAMSS_PROTOBUF_LIBRARY} ${LAMSS_NETSIM_LIBRARY} netsim_messages)
