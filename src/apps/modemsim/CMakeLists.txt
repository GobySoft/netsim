protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(modemsim
  modemsim_main.cpp
  jack_thread.cpp
  groups.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})
set_property(TARGET modemsim PROPERTY CXX_STANDARD 11)
target_link_libraries(modemsim goby_middleware jack)

