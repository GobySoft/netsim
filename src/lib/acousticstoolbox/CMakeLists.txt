protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${project_INC_DIR}
  netsim/acousticstoolbox/environment.proto
  netsim/acousticstoolbox/svp_request_response.proto
  netsim/acousticstoolbox/iBellhop_messages.proto
  )

add_library(netsim_acousticstoolbox SHARED
  ${PROTO_SRCS} ${PROTO_HDRS}
  environment.cpp)

target_link_libraries(netsim_acousticstoolbox
  ${Boost_LIBRARIES}
    goby_moos
    )
  
project_install_lib(netsim_acousticstoolbox)
