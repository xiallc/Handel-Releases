set(MD_SOURCES md_log.c)

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND MD_SOURCES md_win32.c md_threads_windows.c)
else ()
    list(APPEND MD_SOURCES md_linux.c md_threads_posix.c)
endif ()

add_library(MdObjLib OBJECT ${MD_SOURCES})

target_compile_definitions(MdObjLib PUBLIC
        ${GENERAL_COMPILE_DEFS}
        ${PROTOCOL_EXCLUSIONS}
)

target_include_directories(MdObjLib PUBLIC
        ${PROJECT_SOURCE_DIR}/inc
        ${PLX_INCLUDE_DIR}
)