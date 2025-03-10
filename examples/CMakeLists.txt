add_subdirectory(generic)

if (MERCURY)
    add_subdirectory(mercury)
endif ()

if (UDXP)
    add_subdirectory(microdxp)
endif ()

if (SATURN)
    add_subdirectory(saturn)
endif ()

if (STJ)
    add_subdirectory(stj)
endif ()

if (XMAP)
    add_subdirectory(xmap)
endif ()