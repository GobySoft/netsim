include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_goby_executable(TARGET protobuf2bellhop_env
  SOURCES protobuf2bellhop_env.cpp
  LINK_LIBRARIES bellhop_tools
)
