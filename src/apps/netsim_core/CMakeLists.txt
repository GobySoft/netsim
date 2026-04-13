add_goby_executable(TARGET netsim_core
  SOURCES
    netsim_core_main.cpp
    jack_thread.cpp
  # -rdynamic is required so that the shared libraries can access the interthread static objects correctly
  LINK_LIBRARIES -rdynamic goby_zeromq jack netsim_messages
)

if(export_goby_interfaces)
  generate_interfaces_params(netsim_core "-extra-arg=-DNETSIM_MAX_MODEMS=${NETSIM_VISUALIZE_MAX_MODEMS}")
endif()

project_install_bin(netsim_core)
