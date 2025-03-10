add_executable(mca_read mca_read.c $<TARGET_OBJECTS:AssertObjLib>)
target_link_libraries(mca_read handel)

add_executable(mca_read_module mca_read_module.c $<TARGET_OBJECTS:AssertObjLib>)
target_link_libraries(mca_read_module handel)

add_executable(single_read single_read.c $<TARGET_OBJECTS:AssertObjLib>)
target_link_libraries(single_read handel)