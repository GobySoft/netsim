add_subdirectory(apps)
add_subdirectory(lib)
add_subdirectory(test)

if(build_doc)
  find_package(Doxygen REQUIRED dot)
  add_subdirectory(doc)

  
  # doc install
  install(DIRECTORY ${project_BUILD_DIR}/src/doc/html DESTINATION share/doc/netsim)
  install(DIRECTORY ${project_BUILD_DIR}/src/doc/figures DESTINATION share/doc/netsim)

endif()
