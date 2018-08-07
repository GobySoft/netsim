add_executable(cconvolve_test
  test.cpp)

add_executable(cconvolve_test_continuous
  test-continuous.cpp)

target_link_libraries(cconvolve_test ${LAMSS_HENRIK_UTIL_LIBRARY} ${LAMSS_PROTOBUF_LIBRARY} ${PROTOBUF_LIBRARIES} boost_system)
target_link_libraries(cconvolve_test_continuous ${LAMSS_HENRIK_UTIL_LIBRARY} ${LAMSS_PROTOBUF_LIBRARY} ${PROTOBUF_LIBRARIES} boost_system)
