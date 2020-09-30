protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}
  netsim/messages/tool.proto
  netsim/messages/liaison.proto
  netsim/messages/logger.proto
  netsim/messages/config_request.proto
  netsim/messages/manager_config.proto
  netsim/messages/env_bellhop_req.proto
  )

add_library(netsim_messages SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(netsim_messages ${PROTOBUF_LIBRARIES} dccl ${LAMSS_BELLHOP_TOOLS_LIBRARY})
