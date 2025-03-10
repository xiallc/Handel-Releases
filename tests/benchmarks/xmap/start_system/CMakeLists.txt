add_executable(start_system start_system.c $<TARGET_OBJECTS:AssertObjLib>)
target_link_libraries(start_system handel)
target_link_options(start_system PUBLIC "/profile")
if (MSVC)
    target_link_options(start_system PRIVATE
            $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )
endif ()
