cmake_minimum_required(VERSION 3.13)
project(Handel)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
    set(CMAKE_BUILD_TYPE Release CACHE
            STRING "Build type, options are: Debug Release" FORCE)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/xia/${PROJECT_NAME} CACHE PATH "..." FORCE)
    endif ()
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
message(STATUS "Install Path: ${CMAKE_INSTALL_PREFIX}")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/;${PROJECT_SOURCE_DIR}/cmake/functions")
include(BuildUtilities)
include(GitVersion)

check_architecture()
define_options()
apply_business_logic()

MESSAGE(STATUS "Enabled Options:")
validate_options("Devices" "${DEVICES}")
validate_options("Protocols" "${PROTOCOLS}")
validate_options("Libraries" "${LIBRARIES}")
validate_options("Misc" "${MISC_OPTIONS}")

configure_compiler_options()
configure_handel_options()

get_version_from_git()
message(STATUS "Project Version: ${PROJECT_VERSION}")
string(TIMESTAMP BUILD_TIME %Y-%m-%dT%H:%M:%SZ UTC)
configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/templates/xia_version.h.in
        ${CMAKE_SOURCE_DIR}/inc/xia_version.h
)

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    list(JOIN ENABLED_Devices "," DEVICE_STR)
    configure_file(
            ${CMAKE_SOURCE_DIR}/cmake/templates/handel.rc.in
            ${CMAKE_CURRENT_BINARY_DIR}/handel.rc
    )
endif ()

if (PLX)
    find_package(BroadcomAPI REQUIRED)
    install(FILES ${PLX_SHARED_LIB} DESTINATION bin)
    install(FILES
            ${PROJECT_SOURCE_DIR}/externals/plx/drv/PlxSdk.cat
            ${PROJECT_SOURCE_DIR}/externals/plx/drv/xia_plx.inf
            DESTINATION drivers/plx)
    if (X64)
        install(FILES ${PROJECT_SOURCE_DIR}/externals/plx/drv/64-bit/Plx9054.sys
                DESTINATION drivers/plx)
    else ()
        install(FILES ${PROJECT_SOURCE_DIR}/externals/plx/drv/32-bit/Plx9054.sys
                DESTINATION drivers/plx)
    endif ()
endif ()

if (VLD)
    find_package(VisualLeakDetector REQUIRED)
endif ()

MESSAGE(DEBUG "Compiler Flags:")
MESSAGE(DEBUG "    GENERAL_COMPILE_DEFS: ${GENERAL_COMPILE_DEFS}")
MESSAGE(DEBUG "    DEVICE_EXCLUSIONS: ${DEVICE_EXCLUSIONS}")
MESSAGE(DEBUG "    PROTOCOL_EXCLUSIONS: ${PROTOCOL_EXCLUSIONS}")
MESSAGE(DEBUG "    LIBRARY_EXCLUSIONS: ${LIBRARY_EXCLUSIONS}")
MESSAGE(DEBUG "    CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
MESSAGE(DEBUG "    CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
MESSAGE(DEBUG "    CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")

add_subdirectory(inc)
add_subdirectory(src)

if (EXAMPLES)
    add_subdirectory(examples)
endif ()

IF (TESTS)
    add_subdirectory(tests)
endif ()

if (EXISTS ${PROJECT_SOURCE_DIR}/cmake/cpack/config.cmake)
    include(${PROJECT_SOURCE_DIR}/cmake/cpack/config.cmake)
endif ()