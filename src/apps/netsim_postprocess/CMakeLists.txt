protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(netsim_postprocess
  netsim_postprocess_main.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(netsim_postprocess goby goby_zeromq netsim_messages)

if(export_goby_interfaces)
  generate_interfaces(netsim_postprocess)
endif()
