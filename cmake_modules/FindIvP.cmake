find_library(IVP_CORE_LIBRARY NAMES ivpcore
  DOC "The IVP Core library"
  PATHS ~/moos-ivp/lib ${CMAKE_SOURCE_DIR}/../../moos-ivp/lib ${CMAKE_SOURCE_DIR}/../moos-ivp/lib ${IVP_ROOT_DIR}/lib)

 message("IVP Core Library: ${IVP_CORE_LIBRARY}")
 message("MOOS LIBRARY PATH: ${MOOS_LIBRARY_PATH}")


get_filename_component(IVP_LIBRARY_PATH ${IVP_CORE_LIBRARY} PATH)
get_filename_component(IVP_DIR ${IVP_LIBRARY_PATH}/../ ABSOLUTE)

 message("IVP DIR: ${IVP_DIR}")

find_path(IVP_INCLUDE_DIR IvPFunction.h
  DOC "The IVP include directory"
  PATHS ${IVP_DIR}/include/ivp)

find_library(IVP_BEHAVIORS_LIBRARY NAMES behaviors
  DOC "The IVP behaviors library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_BHVUTIL_LIBRARY NAMES bhvutil
  DOC "The IVP bhvutil library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_GENUTIL_LIBRARY NAMES genutil
  DOC "The IVP genutil library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_GEOMETRY_LIBRARY NAMES geometry
  DOC "The IVP geometry library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_HELMIVP_LIBRARY NAMES helmivp
  DOC "The IVP helmivp library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_IPFVIEW_LIBRARY NAMES ipfview
  DOC "The IVP ipfview library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_IVPBUILD_LIBRARY NAMES ivpbuild
  DOC "The IVP ivpbuild library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_LOGIC_LIBRARY NAMES logic
  DOC "The IVP logic library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_LOGUTILS_LIBRARY NAMES logutils
  DOC "The IVP logutils library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_MARINEVIEW_LIBRARY NAMES marineview
  DOC "The IVP marineview library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_MBUTIL_LIBRARY NAMES mbutil
  DOC "The IVP mbutil library"
  PATHS ${IVP_LIBRARY_PATH})


find_library(IVP_CONTACTS_LIBRARY NAMES contacts
  DOC "The IVP contact library"
  PATHS ${IVP_LIBRARY_PATH})

find_library(IVP_GEODESY_LIBRARY NAMES MOOSGeodesy
  DOC "The IVP geodesy library"
  PATHS ${IVP_LIBRARY_PATH})

mark_as_advanced(
  IVP_INCLUDE_DIR
  IVP_CORE_LIBRARY
  IVP_BEHAVIORS_LIBRARY
  IVP_BHVUTIL_LIBRARY
  IVP_GENUTIL_LIBRARY
  IVP_GEOMETRY_LIBRARY
  IVP_HELMIVP_LIBRARY
  IVP_IPFVIEW_LIBRARY
  IVP_IVPBUILD_LIBRARY
  IVP_LOGIC_LIBRARY
  IVP_LOGUTILS_LIBRARY
  IVP_MARINEVIEW_LIBRARY
  IVP_MBUTIL_LIBRARY
  IVP_CONTACTS_LIBRARY
  IVP_GEODESY_LIBRARY
  )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IvP DEFAULT_MSG
  IVP_DIR 
  IVP_INCLUDE_DIR
  IVP_CORE_LIBRARY
  IVP_BEHAVIORS_LIBRARY
  IVP_BHVUTIL_LIBRARY
  IVP_GENUTIL_LIBRARY
  IVP_GEOMETRY_LIBRARY
  IVP_HELMIVP_LIBRARY
#  IVP_IPFVIEW_LIBRARY
  IVP_IVPBUILD_LIBRARY
  IVP_LOGIC_LIBRARY
  IVP_LOGUTILS_LIBRARY
#  IVP_MARINEVIEW_LIBRARY
  IVP_MBUTIL_LIBRARY
  IVP_CONTACTS_LIBRARY
  IVP_GEODESY_LIBRARY
)

if(IVP_FOUND)
  set(IVP_INCLUDE_DIRS
    ${IVP_INCLUDE_DIR}
    "${IVP_INCLUDE_DIR}/.."
    "${IVP_INCLUDE_DIR}/../../MOOS/MOOSGeodesy/libMOOSGeodesy/include"
   )
  set(
    IVP_LIBRARIES 
    ${IVP_CORE_LIBRARY}
    ${IVP_BEHAVIORS_LIBRARY}
    ${IVP_BHVUTIL_LIBRARY}
    ${IVP_GENUTIL_LIBRARY}
    ${IVP_GEOMETRY_LIBRARY}
    ${IVP_HELMIVP_LIBRARY}
#    ${IVP_IPFVIEW_LIBRARY}
    ${IVP_IVPBUILD_LIBRARY}
    ${IVP_LOGIC_LIBRARY}
    ${IVP_LOGUTILS_LIBRARY}
#    ${IVP_MARINEVIEW_LIBRARY}
    ${IVP_MBUTIL_LIBRARY}
    ${IVP_CONTACTS_LIBRARY}
    ${IVP_GEODESY_LIBRARY}
    )

  set(IVP_ROOT_DIR "${IVP_DIR}" CACHE STRING "Path to the root of MOOS-IvP, e.g. /home/me/moos-ivp/ivp" FORCE)
endif()
