protobuf_generate_cpp(
  PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)


add_executable(netsim_tool
  main.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_tool
  goby
  goby_moos
  goby_zeromq
  netsim_messages
  netsim_tcp
  )

if(export_goby_interfaces)
  generate_interfaces(netsim_tool)
endif()
