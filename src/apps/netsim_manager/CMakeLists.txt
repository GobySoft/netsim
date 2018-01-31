protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(netsim_manager
  manager.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_manager goby_middleware ${LAMSS_PROTOBUF_LIBRARY} ${LAMSS_NETSIM_LIBRARY})
