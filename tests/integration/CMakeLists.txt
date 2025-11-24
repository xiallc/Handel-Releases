add_executable(test_basic_memory src/test_basic_memory.c)
target_link_libraries(test_basic_memory handel)

add_executable(test_calibration src/test_calibration.c)
target_link_libraries(test_calibration handel)

add_executable(test_file_handle src/test_file_handle.c)
target_link_libraries(test_file_handle handel)

add_executable(test_api src/test_api.c)
target_link_libraries(test_api handel)
target_include_directories(test_api PUBLIC
        inc/
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/acutest/
)

add_custom_command(TARGET test_api POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/configs
        ${CMAKE_CURRENT_BINARY_DIR}/configs)

if (VLD)
    add_executable(test_memory_leaks src/test_memory_leaks.c)
    target_compile_definitions(test_memory_leaks PUBLIC ${VLD_COMPILE_FLAGS})
    target_include_directories(test_memory_leaks PUBLIC ${VLD_INCLUDE_DIR})
    target_link_directories(test_memory_leaks PUBLIC ${VLD_LIBRARY_DIR})
    target_link_libraries(test_memory_leaks handel vld)
endif ()

if (UDXP)
    add_executable(test_microdxp src/test_microdxp.c)
    target_link_libraries(test_microdxp handel)
    target_include_directories(test_microdxp PUBLIC
            inc/
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/externals/acutest/
    )
endif ()

add_executable(test_preset_run_data src/test_preset_run_data.c)
target_link_libraries(test_preset_run_data handel)
