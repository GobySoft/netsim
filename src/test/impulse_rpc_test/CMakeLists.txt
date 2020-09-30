protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(impulse_rpc_test test.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(impulse_rpc_test goby goby_zeromq netsim_messages)

