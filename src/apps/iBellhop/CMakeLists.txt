add_goby_executable(TARGET iBellhop
  SOURCES
    iBellhop.cpp
    iBellhopMain.cpp
  LINK_LIBRARIES ${MOOS_LIBRARIES} netsim_acousticstoolbox goby_moos ${Boost_LIBRARIES}
)

project_install_bin(iBellhop)
