add_executable(test_basic_memory src/test_basic_memory.c)
target_link_libraries(test_basic_memory handel)

add_executable(test_calibration src/test_calibration.c)
target_link_libraries(test_calibration handel)

add_executable(test_file_handle src/test_file_handle.c)
target_link_libraries(test_file_handle handel)

if (VLD)
    add_executable(test_memory_leaks src/test_memory_leaks.c)
    target_compile_definitions(test_memory_leaks PUBLIC ${VLD_COMPILE_FLAGS})
    target_include_directories(test_memory_leaks PUBLIC ${VLD_INCLUDE_DIR})
    target_link_directories(test_memory_leaks PUBLIC ${VLD_LIBRARY_DIR})
    target_link_libraries(test_memory_leaks handel vld)
endif ()

add_executable(test_preset_run_data src/test_preset_run_data.c)
target_link_libraries(test_preset_run_data handel)
