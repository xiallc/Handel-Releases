add_executable(speed_curve
        speed_curve.c
        $<TARGET_OBJECTS:AssertObjLib>
        $<TARGET_OBJECTS:Usb2ObjLib>
)
target_include_directories(speed_curve PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/microsoft
        ${PROJECT_SOURCE_DIR}/externals/cypress/inc
)
target_link_libraries(speed_curve PUBLIC setupapi)

#-----------------------------------
add_executable(usb2_multi_timing
        multi_timing.c
        $<TARGET_OBJECTS:AssertObjLib>
        $<TARGET_OBJECTS:Usb2ObjLib>
)
target_include_directories(usb2_multi_timing PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/microsoft
        ${PROJECT_SOURCE_DIR}/externals/cypress/inc
)
target_link_libraries(usb2_multi_timing PUBLIC setupapi)

#-----------------------------------
add_executable(usb2_transfer_speeds
        transfer_speeds.c
        $<TARGET_OBJECTS:AssertObjLib>
        $<TARGET_OBJECTS:Usb2ObjLib>
)
target_include_directories(usb2_transfer_speeds PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/microsoft
        ${PROJECT_SOURCE_DIR}/externals/cypress/inc
)
target_link_libraries(usb2_transfer_speeds PUBLIC setupapi)
