# GobyTarget.cmake - convenience functions for building Goby targets
#
# Provides:
#   add_goby_executable(TARGET <name>
#     SOURCES <files>...
#     [PROTOS <proto_files>...]
#     [LINK_LIBRARIES <libs>...]
#     [PROTO_IMPORT_DIRS <dirs>...]
#     [PROTOC_OUT_DIR <dir>])
#
#   add_goby_library(TARGET <name>
#     [SOURCES <files>...]
#     [PROTOS <proto_files>...]
#     [LINK_LIBRARIES <libs>...]
#     [PROTO_IMPORT_DIRS <dirs>...]
#     [PROTOC_OUT_DIR <dir>]
#     [STATIC] [MODULE])
#
# For add_goby_executable, linking against goby is implied.
# Proto files are compiled with protoc --cpp_out + --dccl_out (matching the
# original protobuf_generate_cpp_dccl behaviour).

# Internal helper: compile .proto files with --cpp_out + --dccl_out.
# Sets OUT_VAR in the caller's scope to the list of generated source files.
function(_goby_generate_protos OUT_VAR PROTOC_OUT_DIR PROTOS IMPORT_DIRS)
  # Strategy (mirrors the original FindProtobufLocal.cmake behaviour):
  #
  # 1. Each .proto is first copied into PROTOC_OUT_DIR so that protoc can
  #    resolve cross-proto imports using a stable on-disk path (e.g.
  #    build/include/netsim/acousticstoolbox/environment.proto).
  #
  # 2. The --cpp_out (and --dccl_out) root must be the import-base directory
  #    that is a parent of PROTOC_OUT_DIR, NOT PROTOC_OUT_DIR itself.
  #    Reason: protoc appends the canonical proto name (relative to the -I
  #    base) to the --cpp_out dir when writing output files.  If
  #    PROTOC_OUT_DIR = build/include/netsim/acousticstoolbox and the canonical
  #    name is netsim/acousticstoolbox/environment.proto, passing --cpp_out
  #    build/include/netsim/acousticstoolbox would write the output to
  #    build/include/netsim/acousticstoolbox/netsim/acousticstoolbox/environment.pb.cc
  #    (wrong).  Passing --cpp_out build/include writes it to
  #    build/include/netsim/acousticstoolbox/environment.pb.cc (correct).
  #
  # 3. CMAKE_CURRENT_SOURCE_DIR is intentionally excluded from -I flags so
  #    that the canonical name is derived from the import-base dir (producing
  #    e.g. "netsim/acousticstoolbox/environment.proto") rather than just the
  #    basename ("environment.proto").  Consistent canonical names are required
  #    so that descriptor_table symbols match across all generated .pb.cc files
  #    when one proto imports another.

  # Collect deduplicated import dirs and their absolute paths.
  set(_import_flags)
  set(_seen_dirs)
  set(_all_import_abs)
  foreach(_dir
      ${CMAKE_CURRENT_BINARY_DIR}
      ${IMPORT_DIRS}
      ${GOBY_PROTOBUF_IMPORT_DIRS})
    if(_dir)
      get_filename_component(_abs_dir "${_dir}" ABSOLUTE)
      if(NOT "${_abs_dir}" IN_LIST _seen_dirs)
        list(APPEND _seen_dirs "${_abs_dir}")
        list(APPEND _import_flags -I "${_abs_dir}")
        list(APPEND _all_import_abs "${_abs_dir}")
      endif()
    endif()
  endforeach()

  # Determine the --cpp_out root: the import dir that is a strict parent of
  # PROTOC_OUT_DIR (or PROTOC_OUT_DIR itself when no parent import dir exists).
  get_filename_component(_protoc_out_abs "${PROTOC_OUT_DIR}" ABSOLUTE)
  set(_cpp_out_root "${_protoc_out_abs}")
  foreach(_idir ${_all_import_abs})
    # Check if _protoc_out_abs starts with _idir/ (i.e. _idir is a parent)
    string(FIND "${_protoc_out_abs}" "${_idir}/" _pos)
    if(_pos EQUAL 0)
      set(_cpp_out_root "${_idir}")
      break()
    endif()
  endforeach()

  set(_all_generated)
  foreach(_proto ${PROTOS})
    get_filename_component(_abs_proto "${_proto}" ABSOLUTE)
    get_filename_component(_proto_we  "${_abs_proto}" NAME_WE)

    # Copy the source .proto into PROTOC_OUT_DIR so protoc finds it at the
    # correct path when resolving cross-file imports.
    set(_proto_dest "${_protoc_out_abs}/${_proto_we}.proto")

    # Output files: protoc places them at <cpp_out_root>/<rel_path>/<name>.pb.*
    # where <rel_path> is PROTOC_OUT_DIR relative to _cpp_out_root.
    file(RELATIVE_PATH _rel_subdir "${_cpp_out_root}" "${_protoc_out_abs}")
    if(_rel_subdir)
      set(_pb_h  "${_cpp_out_root}/${_rel_subdir}/${_proto_we}.pb.h")
      set(_pb_cc "${_cpp_out_root}/${_rel_subdir}/${_proto_we}.pb.cc")
    else()
      set(_pb_h  "${_cpp_out_root}/${_proto_we}.pb.h")
      set(_pb_cc "${_cpp_out_root}/${_proto_we}.pb.cc")
    endif()

    # Run protoc with --cpp_out first, then --dccl_out last.
    # This ordering matches the original protobuf_generate_cpp_dccl and ensures
    # that the DCCL plugin can insert into the cpp-generated files.
    add_custom_command(
      OUTPUT "${_pb_h}" "${_pb_cc}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_abs_proto}" "${_proto_dest}"
      COMMAND protobuf::protoc
      ARGS --cpp_out "${_cpp_out_root}"
           "${_proto_dest}"
           ${_import_flags}
           --dccl_out "${_cpp_out_root}"
      DEPENDS "${_abs_proto}" protobuf::protoc
      COMMENT "Running dccl protocol buffer compiler on ${_proto}"
      VERBATIM)

    set_source_files_properties("${_pb_h}" "${_pb_cc}" PROPERTIES GENERATED TRUE)
    list(APPEND _all_generated "${_pb_h}" "${_pb_cc}")
  endforeach()

  set(${OUT_VAR} "${_all_generated}" PARENT_SCOPE)
endfunction()

# add_goby_executable - build a Goby application binary
# Linking against goby is implied; list any additional libs in LINK_LIBRARIES.
function(add_goby_executable)
  cmake_parse_arguments(args
    ""
    "TARGET;PROTOC_OUT_DIR"
    "SOURCES;PROTOS;LINK_LIBRARIES;PROTO_IMPORT_DIRS"
    ${ARGN})

  if(NOT args_TARGET)
    message(FATAL_ERROR "add_goby_executable: TARGET is required")
  endif()

  set(_sources ${args_SOURCES})

  if(args_PROTOS)
    if(args_PROTOC_OUT_DIR)
      set(_protoc_out_dir "${args_PROTOC_OUT_DIR}")
    else()
      set(_protoc_out_dir "${CMAKE_CURRENT_BINARY_DIR}/${args_TARGET}")
    endif()
    file(MAKE_DIRECTORY "${_protoc_out_dir}")

    _goby_generate_protos(
      _proto_generated
      "${_protoc_out_dir}"
      "${args_PROTOS}"
      "${args_PROTO_IMPORT_DIRS}")

    list(APPEND _sources ${_proto_generated})
  endif()

  add_executable(${args_TARGET} ${_sources})

  if(args_PROTOS)
    target_include_directories(${args_TARGET} PRIVATE "${_protoc_out_dir}")
  endif()

  target_link_libraries(${args_TARGET} goby ${args_LINK_LIBRARIES})
endfunction()

# add_goby_library - build a Goby library target (SHARED by default)
# Pass STATIC or MODULE to change the library type.
function(add_goby_library)
  cmake_parse_arguments(args
    "STATIC;MODULE"
    "TARGET;PROTOC_OUT_DIR"
    "SOURCES;PROTOS;LINK_LIBRARIES;PROTO_IMPORT_DIRS"
    ${ARGN})

  if(NOT args_TARGET)
    message(FATAL_ERROR "add_goby_library: TARGET is required")
  endif()

  set(_lib_type SHARED)
  if(args_STATIC)
    set(_lib_type STATIC)
  elseif(args_MODULE)
    set(_lib_type MODULE)
  endif()

  set(_sources ${args_SOURCES})

  if(args_PROTOS)
    if(args_PROTOC_OUT_DIR)
      set(_protoc_out_dir "${args_PROTOC_OUT_DIR}")
    else()
      set(_protoc_out_dir "${CMAKE_CURRENT_BINARY_DIR}/${args_TARGET}")
    endif()
    file(MAKE_DIRECTORY "${_protoc_out_dir}")

    _goby_generate_protos(
      _proto_generated
      "${_protoc_out_dir}"
      "${args_PROTOS}"
      "${args_PROTO_IMPORT_DIRS}")

    list(APPEND _sources ${_proto_generated})
  endif()

  add_library(${args_TARGET} ${_lib_type} ${_sources})

  if(args_PROTOS)
    target_include_directories(${args_TARGET} PUBLIC
      $<BUILD_INTERFACE:${_protoc_out_dir}>)
  endif()

  if(args_LINK_LIBRARIES)
    target_link_libraries(${args_TARGET} ${args_LINK_LIBRARIES})
  endif()
endfunction()
