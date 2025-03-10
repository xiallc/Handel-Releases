add_executable(burst_read
        burst_read.c
        $<TARGET_OBJECTS:PlxObjLib>
        $<TARGET_OBJECTS:AssertObjLib>
)
target_compile_definitions(burst_read PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(burst_read PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PLX_INCLUDE_DIR}
)
target_link_directories(burst_read PUBLIC ${PLX_LIBRARY_DIR})
target_link_libraries(burst_read PUBLIC ${PLX_STATIC_LIB})

#-----------------------------------
add_executable(max_read
        max_read.c
        $<TARGET_OBJECTS:PlxObjLib>
        $<TARGET_OBJECTS:AssertObjLib>
)
target_compile_definitions(max_read PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(max_read PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PLX_INCLUDE_DIR}
)
target_link_directories(max_read PUBLIC ${PLX_LIBRARY_DIR})
target_link_libraries(max_read PUBLIC ${PLX_STATIC_LIB})

#-----------------------------------
add_executable(single_burst_full_mem
        single_burst_full_mem.c
        $<TARGET_OBJECTS:PlxObjLib>
        $<TARGET_OBJECTS:AssertObjLib>
)
target_compile_definitions(single_burst_full_mem PUBLIC ${GENERAL_COMPILE_DEFS})
target_include_directories(single_burst_full_mem PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PLX_INCLUDE_DIR}
)
target_link_directories(single_burst_full_mem PUBLIC ${PLX_LIBRARY_DIR})
target_link_libraries(single_burst_full_mem PUBLIC ${PLX_STATIC_LIB})