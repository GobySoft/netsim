add_goby_library(TARGET netsim_messages
  PROTOS
    netsim.proto
    tool.proto
    liaison.proto
    logger.proto
    core_config.proto
    config_request.proto
    manager_config.proto
    env_bellhop_req.proto
  LINK_LIBRARIES dccl netsim_acousticstoolbox
  PROTO_IMPORT_DIRS ${project_INC_DIR}
  PROTOC_OUT_DIR ${project_INC_DIR}/netsim/messages
)

project_install_lib(netsim_messages)
