add_executable(hqsg-xmap hqsg-xmap.c)
target_link_libraries(hqsg-xmap handel)
install(TARGETS hqsg-xmap RUNTIME)

add_executable(hqsg-xmap-mapping hqsg-xmap-mapping.c)
target_link_libraries(hqsg-xmap-mapping handel)
install(TARGETS hqsg-xmap-mapping RUNTIME)

add_executable(hqsg-xmap-preset hqsg-xmap-preset.c)
target_link_libraries(hqsg-xmap-preset handel)
install(TARGETS hqsg-xmap-preset RUNTIME)

install(FILES xmap_reset_std.ini DESTINATION configs/xmap)
