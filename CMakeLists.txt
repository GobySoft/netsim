project(modemsim C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

# require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# versions
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "10")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(PROJECT_SOVERSION "1")

find_package(ProtobufLocal REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

find_package(DCCL REQUIRED)
get_target_property(DCCL_INCLUDE_DIR dccl INTERFACE_INCLUDE_DIRECTORIES)

message("Using DCCL in ${DCCL_INCLUDE_DIR}")
include_directories("${DCCL_INCLUDE_DIR}")
protobuf_include_dirs("${DCCL_INCLUDE_DIR}")

find_package(GOBY 3 REQUIRED zeromq moos)
message("Using Goby in ${GOBY_INCLUDE_DIR}")
include_directories("${GOBY_INCLUDE_DIR}")
protobuf_include_dirs("${GOBY_INCLUDE_DIR}")

find_package(MOOS 10 REQUIRED)

find_package(Boost REQUIRED serialization)


# create variables for various directories
get_filename_component(project_SRC_DIR src ABSOLUTE)
get_filename_component(project_BUILD_DIR ${CMAKE_BINARY_DIR} ABSOLUTE)
get_filename_component(project_SCRIPTS_DIR scripts ABSOLUTE)

# fetch all the local directories for generated code
get_filename_component(project_LIB_DIR ${project_BUILD_DIR}/lib ABSOLUTE)
get_filename_component(project_BIN_DIR ${project_BUILD_DIR}/bin ABSOLUTE)
get_filename_component(project_INC_DIR ${project_BUILD_DIR}/include ABSOLUTE)
get_filename_component(project_SHARE_DIR ${project_BUILD_DIR}/share ABSOLUTE)
file(MAKE_DIRECTORY ${project_SHARE_DIR})

option(build_doc "Build documentation (requires Doxygen)" OFF)
option(export_goby_interfaces OFF "Build Goby Publish/Subscribe interface files (YML) from code")

if(build_doc)
  set(export_goby_interfaces ON CACHE BOOL "Set export interfaces for documentation" FORCE)
endif()

if(export_goby_interfaces)
  include(GobyClangTool)
  set(YML_OUT_DIR ${project_SHARE_DIR}/netsim/interfaces)
    # limit rendering a smaller number of modems to keep figures readable
  set(NETSIM_VISUALIZE_MAX_MODEMS 3 CACHE STRING "Maximum number of modems to render in netsim_core interfaces file and resulting figures")
endif()

set(NETSIM_MAX_MODEMS 16 CACHE STRING "Maximum number of modems to support")

## set the cmake defaults for libraries and binaries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${project_LIB_DIR} CACHE PATH 
  "Output directory for the dynamic libraries" )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${project_BIN_DIR} CACHE PATH
  "Output directory for the binaries" )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${project_LIB_DIR} CACHE PATH 
  "Output directory for the static libraries (archives)" )

include_directories(${project_INC_DIR})
protobuf_include_dirs(${project_INC_DIR})


## set instructions for `make install`

# override library version for debian sbuild cross-builds
if(CMAKE_CROSSCOMPILING)
  if (EXISTS "/etc/debian_version")
    set(CMAKE_INSTALL_LIBDIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
  endif()
endif()

include(GNUInstallDirs)
# include
install(DIRECTORY ${project_INC_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING REGEX .*\\.h|.*\\.proto)

# bin
macro(project_install_bin target_bin)
  install(TARGETS ${target_bin} DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro()

# lib
macro(project_install_lib target_lib)
  set_property(TARGET ${target_lib} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    $<BUILD_INTERFACE:${project_INC_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>  # <prefix>/include
    )

  set_target_properties(${target_lib} PROPERTIES VERSION "${PROJECT_VERSION}" SOVERSION "${PROJECT_SOVERSION}")
  
  install(TARGETS ${target_lib} EXPORT netsim-config
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}) 
  list(APPEND PROJECT_LIBRARY_LIST ${target_lib})
  set(PROJECT_LIBRARY_LIST ${PROJECT_LIBRARY_LIST} CACHE INTERNAL "project_library_list")
endmacro()

install(DIRECTORY ${project_SHARE_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share)

unset(PROJECT_LIBRARY_LIST CACHE)

add_subdirectory(src)

if(export_goby_interfaces)
  generate_interfaces_figure(missions/netsim_udp_deployment.yml ${YML_OUT_DIR} ${project_SHARE_DIR}/doc/netsim/netsim_udp_interfaces.pdf "") 
  generate_interfaces_figure(missions/netsim_core_deployment.yml ${YML_OUT_DIR} ${project_SHARE_DIR}/doc/netsim/netsim_core_interfaces_no_disconnect.pdf "-no-disconnected;-splines=polyline;") 
  generate_interfaces_figure(missions/netsim_core_deployment.yml ${YML_OUT_DIR} ${project_SHARE_DIR}/doc/netsim/netsim_core_interfaces.pdf "") 
  generate_interfaces_figure(missions/netsim_core_deployment.yml ${YML_OUT_DIR} ${CMAKE_BINARY_DIR}/src/doc/figures/netsim_core_interfaces.svg "") 
endif()

## configure to build/include
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${project_SRC_DIR}/lib
  src/lib/messages/*.proto
  src/lib/messages/*.h
  src/lib/tcp/*.h
  src/lib/acousticstoolbox/*.h
  src/lib/core/*.h
  )
foreach(I ${INCLUDE_FILES})
  configure_file(${project_SRC_DIR}/lib/${I} ${project_INC_DIR}/netsim/${I} @ONLY)
endforeach()


# configuration export - installed version
install(EXPORT netsim-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/netsim)
# configuration export - local build version
export(TARGETS ${PROJECT_LIBRARY_LIST} FILE ${CMAKE_BINARY_DIR}/netsim-config.cmake)
 
