protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}/messages
  tool.proto
  liaison.proto
  logger.proto
  config_request.proto
  manager_config.proto
  env_bellhop_req.proto
  )

add_library(netsim_messages SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(netsim_messages ${PROTOBUF_LIBRARIES} dccl ${LAMSS_BELLHOP_TOOLS_LIBRARY})
