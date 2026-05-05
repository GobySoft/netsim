add_goby_executable(TARGET netsim_postprocess
  SOURCES netsim_postprocess_main.cpp
  PROTOS config.proto
  LINK_LIBRARIES goby_zeromq netsim_messages
)

if(export_goby_interfaces)
  generate_interfaces(netsim_postprocess)
endif()

project_install_bin(netsim_postprocess)
