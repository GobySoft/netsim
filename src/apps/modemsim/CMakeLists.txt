protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(modemsim
  modemsim_main.cpp
  jack_thread.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(modemsim goby_middleware jack)

