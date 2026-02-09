add_executable(test_mdxp_usb2 src/test_mdxp_usb2.c)
target_include_directories(test_mdxp_usb2 PUBLIC
        inc
        ${PROJECT_SOURCE_DIR}/tests/integration/inc
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/acutest
)
target_link_libraries(test_mdxp_usb2 handel)

add_executable(test_microdxp src/test_microdxp_api.c)
target_link_libraries(test_microdxp handel)
target_include_directories(test_microdxp PUBLIC
        inc
        ${PROJECT_SOURCE_DIR}/tests/integration/inc
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/acutest
)