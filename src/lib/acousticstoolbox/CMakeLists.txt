protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}
  netsim/acousticstoolbox/environment.proto
  netsim/acousticstoolbox/svp_request_response.proto
  netsim/acousticstoolbox/iBellhop_messages.proto
  # to do - update
  netsim/acousticstoolbox/lamss_environment_update.proto
  )

add_library(netsim_acousticstoolbox SHARED
  ${PROTO_SRCS} ${PROTO_HDRS}
  environment.cpp)

target_link_libraries(netsim_acousticstoolbox
  ${Boost_LIBRARIES}
    goby_moos
    )

set_property(TARGET netsim_acousticstoolbox APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${project_INC_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

set_target_properties(netsim_acousticstoolbox PROPERTIES VERSION "${NETSIM_VERSION}" SOVERSION "${NETSIM_SOVERSION}")

install(TARGETS netsim_acousticstoolbox EXPORT netsim-config
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib) 

list(APPEND NETSIM_LIBRARY_LIST netsim_acousticstoolbox)
set(NETSIM_LIBRARY_LIST ${NETSIM_LIBRARY_LIST} CACHE INTERNAL "NETSIM_LIBRARY_LIST")
