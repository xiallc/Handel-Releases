add_executable(hqsg-stj-list hqsg-stj-list.c)
target_link_libraries(hqsg-stj-list handel)
sign_code(hqsg-stj-list)
install(TARGETS hqsg-stj-list RUNTIME)
