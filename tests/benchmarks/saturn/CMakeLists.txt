add_executable(transfer_speeds transfer_speeds.c $<TARGET_OBJECTS:AssertObjLib>)
target_compile_definitions(transfer_speeds PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(transfer_speeds PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(transfer_speeds handel)

#-----------------------------------
add_executable(multi_timing multi_timing.c $<TARGET_OBJECTS:AssertObjLib>)
target_compile_definitions(multi_timing PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(multi_timing PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(multi_timing handel)

#-----------------------------------
add_executable(peaking_time_change peaking_time_change.c $<TARGET_OBJECTS:AssertObjLib>)
target_compile_definitions(peaking_time_change PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(peaking_time_change PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(peaking_time_change handel)

#-----------------------------------
add_executable(change_pt_same_decimation change_pt_same_decimation.c)
target_compile_definitions(change_pt_same_decimation PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(change_pt_same_decimation PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(change_pt_same_decimation handel)

#-----------------------------------
add_executable(siint_memory_leak siint_memory_leak.c)
target_compile_definitions(siint_memory_leak PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(siint_memory_leak PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(siint_memory_leak handel)

#-----------------------------------
add_executable(siint_disconnect_mem_leak siint_disconnect_mem_leak.c)
target_compile_definitions(siint_disconnect_mem_leak PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(siint_disconnect_mem_leak PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(siint_disconnect_mem_leak handel)