# Locate and configure the Google Protocol Buffers library.
# Defers to the new CMake config shipped with Protobuf

# Adds the following functions:
# PROTOBUF_GENERATE_CPP_DCCL
# PROTOBUF_INCLUDE_DIRS

function(PROTOBUF_INCLUDE_DIRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_INCLUDE_DIRS() called without any directories")
    return()
  endif()  

  foreach(DIR ${ARGN})
    set(ALL_PROTOBUF_INCLUDE_DIRS "-I${DIR};${ALL_PROTOBUF_INCLUDE_DIRS}" PARENT_SCOPE)
  endforeach()
endfunction()

function(PROTOBUF_GENERATE_CPP_DCCL SRCS HDRS CPP_OUT_DIR)
  protobuf_include_dirs(${CMAKE_CURRENT_BINARY_DIR})

  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP_DCCL() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})

    # full file name (relative to current source directory)
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    # relative file name (need to do this in case an absolute path is passed in)
    STRING(REGEX REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/" "" REL_FIL ${ABS_FIL})
    # name without extension, e.g. for "foo.proto", ${FIL_WE} is "foo"
    get_filename_component(FIL_WE ${REL_FIL} NAME_WE)
    # relative directory to current source directory, e.g. for "dir/foo.proto", ${FIL_DIR} is "dir"
    get_filename_component(FIL_DIR ${REL_FIL} DIRECTORY BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})     
    
    # if not empty (that is, proto is in this directory)
    if(FIL_DIR)
      set(FULL_OUT_DIR "${CPP_OUT_DIR}/${FIL_DIR}")
    else() # avoid an extra /
      set(FULL_OUT_DIR "${CPP_OUT_DIR}")
    endif()
    
    list(APPEND ${SRCS} "${FULL_OUT_DIR}/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${FULL_OUT_DIR}/${FIL_WE}.pb.h")   

    # we need the output proto in the same directory as the generated code to make sure we generate the relative
    # files correctly for later inclusion into other protos (Protobuf is picky about this)
    set(FIL_DEST ${FULL_OUT_DIR}/${FIL_WE}.proto)

    # copy proto to destination 
    configure_file(${FIL_WE}.proto ${FIL_DEST} COPYONLY)
    
    add_custom_command(
      OUTPUT "${FULL_OUT_DIR}/${FIL_WE}.pb.cc"
             "${FULL_OUT_DIR}/${FIL_WE}.pb.h"
      COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --cpp_out ${CPP_OUT_DIR} ${FIL_DEST} ${ALL_PROTOBUF_INCLUDE_DIRS} --dccl_out ${CPP_OUT_DIR} 
      DEPENDS ${FIL_DEST}
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)

  include_directories(${CPP_OUT_DIR})

endfunction()

# if no Protobuf found already (by imported library), prefer CMake included with Protobuf
if(NOT Protobuf_FOUND)
  find_package(protobuf QUIET CONFIG)
endif()

# if that fails, use the CMake shipped module
if(NOT Protobuf_FOUND)
  find_package(Protobuf REQUIRED MODULE)
  set(protobuf_VERSION ${Protobuf_VERSION})
endif()

if(Protobuf_FOUND)
  set(ProtobufLocal_FOUND True)
endif()
