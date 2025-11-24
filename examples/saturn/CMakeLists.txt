add_executable(hqsg-saturn hqsg-saturn.c)
target_link_libraries(hqsg-saturn handel)
sign_code(hqsg-saturn)
install(TARGETS hqsg-saturn RUNTIME)

install(FILES saturn.ini DESTINATION configs/saturn)
if (EPP)
    install(FILES saturn_reset_epp.ini DESTINATION configs/saturn)
endif ()
