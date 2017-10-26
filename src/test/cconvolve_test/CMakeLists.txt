add_executable(cconvolve_test
  test.cpp)

target_link_libraries(cconvolve_test ${LAMSS_HENRIK_UTIL_LIBRARY} ${LAMSS_PROTOBUF_LIBRARY} ${PROTOBUF_LIBRARIES} boost_system)

