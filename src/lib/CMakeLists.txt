unset(NETSIM_LIBRARY_LIST CACHE)

add_subdirectory(moos_translator_plugin)
add_subdirectory(liaison)
add_subdirectory(tcp)
add_subdirectory(messages)
add_subdirectory(acousticstoolbox)


export(TARGETS ${NETSIM_LIBRARY_LIST} FILE ${CMAKE_BINARY_DIR}/netsim-config.cmake)
