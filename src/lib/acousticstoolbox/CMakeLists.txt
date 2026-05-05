add_goby_library(TARGET netsim_acousticstoolbox
  SOURCES environment.cpp
  PROTOS
    environment.proto
    svp_request_response.proto
    iBellhop_messages.proto
  LINK_LIBRARIES ${Boost_LIBRARIES} goby_moos
  PROTO_IMPORT_DIRS ${project_INC_DIR}
  PROTOC_OUT_DIR ${project_INC_DIR}/netsim/acousticstoolbox
)

project_install_lib(netsim_acousticstoolbox)
