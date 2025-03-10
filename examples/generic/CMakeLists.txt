add_executable(handel-error_code handel-error_code.c)
target_link_libraries(handel-error_code handel)
install(TARGETS handel-error_code RUNTIME)

add_executable(handel-gap_time handel-gap_time.c)
target_link_libraries(handel-gap_time handel)
install(TARGETS handel-gap_time RUNTIME)

add_executable(handel-multi_module handel-multi_module.c)
target_link_libraries(handel-multi_module handel)
install(TARGETS handel-multi_module RUNTIME)

add_executable(handel-preset_run handel-preset_run.c)
target_link_libraries(handel-preset_run handel)
install(TARGETS handel-preset_run RUNTIME)

add_executable(handel-sca handel-sca.c)
target_link_libraries(handel-sca handel)
install(TARGETS handel-sca RUNTIME)

add_executable(infinite_read_loops infinite_read_loops.c)
target_link_libraries(infinite_read_loops handel)
install(TARGETS infinite_read_loops RUNTIME)
