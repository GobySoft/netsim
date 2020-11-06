add_subdirectory(apps)
add_subdirectory(lib)
add_subdirectory(test)

if(build_doc)
  find_package(Doxygen REQUIRED dot)
  add_subdirectory(doc)
endif()
