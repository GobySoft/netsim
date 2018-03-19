protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}/messages
  tool.proto
  )

add_library(modemsim_messages SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(modemsim_messages ${PROTOBUF_LIBRARIES} dccl)
