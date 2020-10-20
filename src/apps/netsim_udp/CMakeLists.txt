
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)


add_executable(netsim_udp
  main.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_udp goby goby_zeromq netsim_messages)


if(export_goby_interfaces)
  generate_interfaces(netsim_udp)
endif()

project_install_bin(netsim_udp)

