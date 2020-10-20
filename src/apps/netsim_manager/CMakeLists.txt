add_executable(netsim_manager
  manager.cpp)

target_link_libraries(netsim_manager goby goby_zeromq netsim_tcp netsim_messages)
if(export_goby_interfaces)
  generate_interfaces(netsim_manager)
endif()

project_install_bin(netsim_manager)
