add_goby_executable(TARGET netsim_tool
  SOURCES main.cpp
  PROTOS config.proto
  LINK_LIBRARIES goby_moos goby_zeromq netsim_messages netsim_tcp
)

if(export_goby_interfaces)
  generate_interfaces(netsim_tool)
endif()

project_install_bin(netsim_tool)
