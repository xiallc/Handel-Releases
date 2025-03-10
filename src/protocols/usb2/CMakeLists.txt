if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    add_library(Usb2ObjLib OBJECT xia_usb2.c)
    set(USB2_LIB setupapi PARENT_SCOPE)
else ()
    add_library(Usb2ObjLib OBJECT xia_usb_linux.c)
    set(USB2_LIB usb PARENT_SCOPE)
endif ()

target_include_directories(Usb2ObjLib PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PROJECT_SOURCE_DIR}/externals/microsoft
        ${PROJECT_SOURCE_DIR}/externals/cypress/inc
)

target_compile_definitions(Usb2ObjLib PUBLIC ${GENERAL_COMPILE_DEFS})

set(USB2_DRIVER_BASE_PATH ${PROJECT_SOURCE_DIR}/externals/cypress/drv/usb2)
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    if (X64)
        set(USB2_DRIVER_PATH ${USB2_DRIVER_BASE_PATH}/x64)
    else ()
        set(USB2_DRIVER_PATH ${USB2_DRIVER_BASE_PATH}/x86)
    endif ()
    install(DIRECTORY ${USB2_DRIVER_PATH}/ DESTINATION drivers/cypress/usb2)
else ()
    install(FILES ${USB2_DRIVER_BASE_PATH}/linux/99-xia-usb.rules
            DESTINATION etc/udev/rules.d/)
endif ()