add_library(netsim_tcp SHARED
  tcp_client.cpp
  tcp_server.cpp
  tcp_session.cpp)

target_link_libraries(netsim_tcp ${Boost_LIBRARIES} ${GOBY_LIBRARIES} b64)


set_property(TARGET netsim_tcp APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${project_INC_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

set_target_properties(netsim_tcp PROPERTIES VERSION "${NETSIM_VERSION}" SOVERSION "${NETSIM_SOVERSION}")

install(TARGETS netsim_tcp EXPORT netsim-config
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib) 

list(APPEND NETSIM_LIBRARY_LIST netsim_tcp)
set(NETSIM_LIBRARY_LIST ${NETSIM_LIBRARY_LIST} CACHE INTERNAL "NETSIM_LIBRARY_LIST")
