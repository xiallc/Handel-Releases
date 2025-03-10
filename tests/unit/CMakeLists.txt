add_executable(test_handel src/test_handel.c)
target_link_libraries(test_handel handel)
target_include_directories(test_handel PUBLIC
        inc/
        ${PROJECT_SOURCE_DIR}/externals/acutest/
)

add_custom_command(TARGET test_handel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/helpers
                ${CMAKE_CURRENT_BINARY_DIR}/helpers)

if (UDXP)
    add_executable(test_microdxp src/test_microdxp.c)
    target_link_libraries(test_microdxp handel)
    target_include_directories(test_microdxp PUBLIC
            inc/
            ${PROJECT_SOURCE_DIR}/externals/acutest/
    )
endif ()