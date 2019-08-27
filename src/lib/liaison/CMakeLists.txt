add_library(netsim_liaison SHARED
  liaison_netsim.cpp
  liaison_load.cpp
  )

find_path(Wt_INCLUDE_DIR Wt/WObject)
find_library(Wt_LIBRARY NAMES wt DOC "The Wt C++ Web library")

target_link_libraries(netsim_liaison goby goby_zeromq ${Wt_LIBRARY} netsim_messages ${LAMSS_PROTOBUF_LIBRARY})
