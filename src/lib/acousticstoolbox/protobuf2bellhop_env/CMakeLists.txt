include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(protobuf2bellhop_env protobuf2bellhop_env.cpp)
target_link_libraries(protobuf2bellhop_env bellhop_tools   ${PROTOBUF_LIBRARIES}
)

