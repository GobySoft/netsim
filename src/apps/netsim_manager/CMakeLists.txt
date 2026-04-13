add_goby_executable(TARGET netsim_manager
  SOURCES manager.cpp
  LINK_LIBRARIES goby_zeromq netsim_tcp netsim_messages
)

if(export_goby_interfaces)
  generate_interfaces(netsim_manager)
endif()

project_install_bin(netsim_manager)
