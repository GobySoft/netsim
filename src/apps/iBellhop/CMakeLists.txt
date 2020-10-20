add_executable(iBellhop
  iBellhop.cpp
  iBellhopMain.cpp
  )
      
target_link_libraries(iBellhop
  ${MOOS_LIBRARIES}
  netsim_acousticstoolbox
  goby_moos
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
)

project_install_bin(iBellhop)
