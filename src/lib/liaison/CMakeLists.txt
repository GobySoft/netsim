find_path(Wt_INCLUDE_DIR Wt/WObject)
find_library(Wt_LIBRARY NAMES wt DOC "The Wt C++ Web library")

if(Wt_LIBRARY)
  add_library(netsim_liaison SHARED
    liaison_netsim.cpp
    liaison_load.cpp
    )
  
  target_link_libraries(netsim_liaison goby goby_zeromq ${Wt_LIBRARY} netsim_messages ${LAMSS_PROTOBUF_LIBRARY})
else()
  message("Not building netsim_liaison since libwt-dev is not installed")
endif()
