add_executable(handel-error_code handel-error_code.c)
target_link_libraries(handel-error_code handel)
sign_code(handel-error_code)
install(TARGETS handel-error_code RUNTIME)

add_executable(handel-gap_time handel-gap_time.c)
target_link_libraries(handel-gap_time handel)
sign_code(handel-gap_time)
install(TARGETS handel-gap_time RUNTIME)

add_executable(handel-multi_module handel-multi_module.c)
target_link_libraries(handel-multi_module handel)
sign_code(handel-multi_module)
install(TARGETS handel-multi_module RUNTIME)

add_executable(handel-preset_run handel-preset_run.c)
target_link_libraries(handel-preset_run handel)
sign_code(handel-preset_run)
install(TARGETS handel-preset_run RUNTIME)

add_executable(handel-sca handel-sca.c)
target_link_libraries(handel-sca handel)
sign_code(handel-sca)
install(TARGETS handel-sca RUNTIME)

add_executable(infinite_read_loops infinite_read_loops.c)
target_link_libraries(infinite_read_loops handel)
sign_code(infinite_read_loops)
install(TARGETS infinite_read_loops RUNTIME)
