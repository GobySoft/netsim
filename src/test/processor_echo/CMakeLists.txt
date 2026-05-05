add_goby_library(TARGET netsim_processor_echo_plugin
  SOURCES processor.cpp
  PROTOS processor_config.proto
  LINK_LIBRARIES netsim_messages
  PROTO_IMPORT_DIRS ${project_INC_DIR}
)

if(export_goby_interfaces)
  generate_interfaces_params(netsim_processor_echo_plugin "-extra-arg=-DNETSIM_MAX_MODEMS=${NETSIM_VISUALIZE_MAX_MODEMS}")
endif()
