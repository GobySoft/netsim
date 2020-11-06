protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}
  processor_config.proto
  )

add_library(netsim_processor_echo_plugin SHARED processor.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_processor_echo_plugin netsim_messages)

if(export_goby_interfaces)
  generate_interfaces(netsim_processor_echo_plugin)
endif()
