if (PLX AND ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    add_subdirectory(plx)
endif ()

if (SATURN AND ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    add_subdirectory(saturn)
endif ()

if (USB2 AND ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    add_subdirectory(usb2)
endif ()

if (XMAP)
    add_subdirectory(xmap)
endif ()