protobuf_generate_cpp(
  PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)


add_executable(netsim_tool
  main.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_tool
  goby
  goby_zeromq
  netsim_messages
  ${LAMSS_PROTOBUF_LIBRARY}
  ${LAMSS_NETSIM_LIBRARY}
  )

