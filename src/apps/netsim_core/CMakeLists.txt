add_executable(netsim_core
  netsim_core_main.cpp
  jack_thread.cpp)

# -rdynamic is required so that the shared libraries can access the interthread static objects correctly 
target_link_libraries(netsim_core "-rdynamic" goby goby_zeromq jack netsim_messages)

if(export_goby_interfaces)
  generate_interfaces_params(netsim_core "-extra-arg=-DNETSIM_MAX_MODEMS=${NETSIM_VISUALIZE_MAX_MODEMS}")
endif()

project_install_bin(netsim_core)
