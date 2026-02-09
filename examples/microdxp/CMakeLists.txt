add_executable(hqsg-microdxp hqsg-microdxp.c)
target_link_libraries(hqsg-microdxp
        handel
        $<$<C_COMPILER_ID:GNU>:"m">
)
sign_code(hqsg-microdxp)
install(TARGETS hqsg-microdxp RUNTIME)

add_executable(hqsg-microdxp-passthrough hqsg-microdxp-passthrough.c)
target_link_libraries(hqsg-microdxp-passthrough handel)
sign_code(hqsg-microdxp-passthrough)
install(TARGETS hqsg-microdxp-passthrough RUNTIME)

add_executable(hqsg-microdxp-snapshot hqsg-microdxp-snapshot.c)
target_link_libraries(hqsg-microdxp-snapshot handel)
sign_code(hqsg-microdxp-snapshot)
install(TARGETS hqsg-microdxp-snapshot RUNTIME)

add_executable(hqsg-microdxp-vega hqsg-microdxp-vega.c)
target_link_libraries(hqsg-microdxp-vega handel)
sign_code(hqsg-microdxp-vega)
install(TARGETS hqsg-microdxp-vega RUNTIME)

install(FILES
        microdxp-shell/microdxp.py
        microdxp-shell/microdxp_shell.py
        microdxp-shell/usb_serial_command.py
        microdxp-shell/utils.py
        microdxp-shell/README.md
        DESTINATION microdxp-shell/
)

install(FILES
        microdxp_std.ini
        microdxp_usb2.ini
        DESTINATION configs/microdxp
)

install(FILES microdxp-shell/ini/template.ini
        RENAME microdxp_shell.ini
        DESTINATION configs/microdxp
)