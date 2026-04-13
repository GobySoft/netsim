add_goby_executable(TARGET impulse_rpc_test
  SOURCES test.cpp
  PROTOS config.proto
  LINK_LIBRARIES goby_zeromq netsim_messages
)
