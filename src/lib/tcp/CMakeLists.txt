add_library(netsim_tcp SHARED
  tcp_client.cpp
  tcp_server.cpp
  tcp_session.cpp)

target_link_libraries(netsim_tcp ${Boost_LIBRARIES} ${GOBY_LIBRARIES} b64)


set_property(TARGET netsim_tcp APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${project_INC_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

project_install_lib(netsim_tcp)
