add_executable(netsim_manager
  manager.cpp)

target_link_libraries(netsim_manager goby goby_zeromq netsim_tcp netsim_messages)
