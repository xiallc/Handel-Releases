add_executable(hqsg-mercury-oem hqsg-mercury-oem.c)
target_link_libraries(hqsg-mercury-oem handel)
install(TARGETS hqsg-mercury-oem RUNTIME)