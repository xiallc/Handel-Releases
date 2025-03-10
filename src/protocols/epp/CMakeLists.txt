if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    set(EPP_INC_DIR ${PROJECT_SOURCE_DIR}/externals/dlportio/inc PARENT_SCOPE)
    set(EPP_LIB_DIR ${PROJECT_SOURCE_DIR}/externals/dlportio/lib PARENT_SCOPE)
    set(EPP_LIB dlportio PARENT_SCOPE)

    add_library(EppObjLib OBJECT xia_epp.c)
    target_include_directories(EppObjLib PUBLIC
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/externals/dlportio/inc
    )

    install(FILES ${PROJECT_SOURCE_DIR}/externals/dlportio/bin/dlportio.dll
            DESTINATION bin)
    install(FILES ${PROJECT_SOURCE_DIR}/externals/dlportio/drv/dlportio.sys
            DESTINATION drivers/dlportio/)
else ()
    add_library(EppObjLib OBJECT xia_epp_linux.c)
endif ()

