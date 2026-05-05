add_goby_library(TARGET netsim_tcp
  SOURCES
    tcp_client.cpp
    tcp_server.cpp
    tcp_session.cpp
  LINK_LIBRARIES ${Boost_LIBRARIES} goby b64
)

set_property(TARGET netsim_tcp APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${project_INC_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

project_install_lib(netsim_tcp)
