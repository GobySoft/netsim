add_goby_library(TARGET netsim_moos_plugin
  SOURCES moos_plugin.cpp
  LINK_LIBRARIES goby goby_moos goby_zeromq ${LAMSS_PROTOBUF_LIBRARY} netsim_messages
)

configure_file(netsim_goby_moos_gateway.in ${project_BIN_DIR}/netsim_goby_moos_gateway @ONLY)

install(PROGRAMS netsim_goby_moos_gateway.install DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME netsim_goby_moos_gateway)


if(export_goby_interfaces)
  generate_interfaces(netsim_moos_plugin)
endif()

project_install_lib(netsim_moos_plugin)
