add_executable(hqsg-mercury-oem hqsg-mercury-oem.c)
target_link_libraries(hqsg-mercury-oem handel)
sign_code(hqsg-mercury-oem)
install(TARGETS hqsg-mercury-oem RUNTIME)