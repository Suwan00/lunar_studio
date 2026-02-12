# Plugin installation
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/objs/LutiCamera/Camera.plugin
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/plugins)

# Also copy to ISISDATA plugins directory
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/objs/LutiCamera/Camera.plugin
        DESTINATION $ENV{ISISDATA}/plugins)
