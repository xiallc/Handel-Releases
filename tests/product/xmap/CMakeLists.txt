add_executable(test_full_buffers src/test_full_buffers.c)
target_link_libraries(test_full_buffers handel)

add_executable(test_list-mode_read src/test_list-mode_read.c)
target_link_libraries(test_list-mode_read handel)

add_executable(test_state_machine_stress src/test_state_machine_stress.c)
target_link_libraries(test_state_machine_stress handel)