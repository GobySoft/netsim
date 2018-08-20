add_executable(netsim_manager
  manager.cpp)

target_link_libraries(netsim_manager goby_middleware ${LAMSS_PROTOBUF_LIBRARY} ${LAMSS_NETSIM_LIBRARY} netsim_messages)
