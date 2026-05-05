add_goby_executable(TARGET netsim_udp
  SOURCES main.cpp
  PROTOS config.proto
  LINK_LIBRARIES goby_zeromq netsim_messages
)

if(export_goby_interfaces)
  generate_interfaces(netsim_udp)
endif()

project_install_bin(netsim_udp)
