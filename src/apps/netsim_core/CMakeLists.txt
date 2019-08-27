protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)


add_executable(netsim_core
  netsim_core_main.cpp
  jack_thread.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_core goby goby_zeromq jack ${LAMSS_HENRIK_UTIL_LIBRARY} ${LAMSS_PROTOBUF_LIBRARY} netsim_messages)

