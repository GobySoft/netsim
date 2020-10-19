protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}
  netsim/messages/netsim.proto
  netsim/messages/tool.proto
  netsim/messages/liaison.proto
  netsim/messages/logger.proto
  netsim/messages/core_config.proto
  netsim/messages/config_request.proto
  netsim/messages/manager_config.proto
  netsim/messages/env_bellhop_req.proto
  )

add_library(netsim_messages SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(netsim_messages ${PROTOBUF_LIBRARIES} dccl netsim_acousticstoolbox)


set_property(TARGET netsim_messages APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${project_INC_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

set_target_properties(netsim_messages PROPERTIES VERSION "${NETSIM_VERSION}" SOVERSION "${NETSIM_SOVERSION}")

install(TARGETS netsim_messages EXPORT netsim-config
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib) 

list(APPEND NETSIM_LIBRARY_LIST netsim_messages)
set(NETSIM_LIBRARY_LIST ${NETSIM_LIBRARY_LIST} CACHE INTERNAL "NETSIM_LIBRARY_LIST")
