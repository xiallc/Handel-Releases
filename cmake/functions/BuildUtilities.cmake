# SPDX-License-Identifier: Apache-2.0

# Copyright 2024 XIA LLC, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# @file build_utilities.cmake
# @brief Collects useful cmake functions for various purposes

# Applies the business logic to the flags to ensure that we don't set anything
# that doesn't make sense. Ex. Build xMap on Linux.
# Note: We set the protocols and libraries twice because we need those values both
#       locally and in the parent scope. Setting just the parent scope prevents the
#       checks on the devices from executing properly.
#       See https://stackoverflow.com/q/34028300

function(apply_business_logic)
    if (${CMAKE_HOST_UNIX})
        set(EPP OFF)
        set(PLX OFF)
        set(SERIAL OFF)
        set(UDXPS OFF)
        set(STJ OFF)
        set(XMAP OFF)
        set(USB OFF)
        set(VLD OFF)
        set(XUP OFF)
        message(STATUS "STJ;UDXPS;XMAP;EPP;PLX;SERIAL;USB;VLD;XUP not supported on Linux")
    endif ()

    if (${CMAKE_HOST_WIN32})
        if (${CMAKE_C_COMPILER_ARCHITECTURE_ID} MATCHES "x64")
            set(EPP OFF)
            set(SERIAL OFF)
            set(USB OFF)
            message(STATUS "EPP;SERIAL;USB not supported on 64-bit Windows")
        else ()
            set(X64 OFF PARENT_SCOPE)
        endif ()
    endif ()

    if (ULTRALO)
        message(STATUS "ULTRALO allows only UDXP and USB2")
    endif ()

    if ((MERCURY AND NOT USB2) OR ULTRALO)
        set(MERCURY OFF)
        message(STATUS "MERCURY requires USB2")
    endif ()

    if ((SATURN AND (NOT EPP AND NOT USB AND NOT USB2)) OR ULTRALO)
        set(SATURN OFF)
        message(STATUS "SATURN requires EPP, USB, or USB2")
    endif ()

    if ((STJ AND NOT PLX) OR ULTRALO)
        set(STJ OFF)
        message(STATUS "STJ requires PLX")
    endif ()

    if (UDXP AND (NOT SERIAL AND NOT USB2))
        set(UDXP OFF)
        message(STATUS "UDXP requires SERIAL or USB2")
    endif ()

    if ((UDXPS AND (NOT XUP OR (NOT SERIAL AND NOT USB2)))
            OR (NOT EXISTS ${PROJECT_SOURCE_DIR}/src/devices/udxp/udxps.c
            AND NOT EXISTS ${PROJECT_SOURCE_DIR}/src/devices/udxp/udxps_psl.c))
        set(UDXPS OFF)
        message(STATUS "UDXPS requires SERIAL or USB2 and Windows")
    endif ()

    if (NOT UDXP AND NOT UDXPS)
        set(XUP OFF)
        message(STATUS "XUP requires UDXP or UDXPS")
    endif ()

    if ((XMAP AND NOT PLX) OR ULTRALO)
        set(XMAP OFF)
        message(STATUS "XMAP requires PLX")
    endif ()

    if ((NOT XMAP AND NOT STJ) OR ULTRALO)
        set(PLX OFF)
        message(STATUS "PLX requires XMAP or STJ")
    endif ()

    list(APPEND options ${DEVICES} ${PROTOCOLS} ${LIBRARIES} ${MISC_OPTIONS})
    foreach (option ${options})
        get_property(enabled VARIABLE PROPERTY ${option})
        if (enabled)
            set(${option} ON PARENT_SCOPE)
        else ()
            set(${option} OFF PARENT_SCOPE)
        endif ()
    endforeach ()
endfunction()

# Checks the system and compilers to ensure that we have the proper environments.
function(check_architecture)
    if (NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux" AND NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
        message(FATAL_ERROR "${CMAKE_HOST_SYSTEM_NAME} is not supported!")
    endif ()
endfunction()

# Defines all the options that the we need to handle
function(define_options)
    # Define the devices
    set(DEVICES MERCURY SATURN STJ UDXP UDXPS ULTRALO XMAP PARENT_SCOPE)
    option(MERCURY "Include Mercury/Mercury-4 support" OFF)
    option(SATURN "Include Saturn support" OFF)
    option(STJ "Include STJ support" OFF)
    option(UDXP "Include microDXP support" ON)
    option(UDXPS "Include microDXP setup support" OFF)
    option(ULTRALO "Include UltraLo-1800. Excludes all other devices." OFF)
    option(XMAP "Include xMAP support" OFF)

    # Define the protocols
    set(PROTOCOLS EPP PLX SERIAL USB USB2 PARENT_SCOPE)
    option(EPP "Include EPP driver" OFF)
    option(PLX "Include PLX driver" OFF)
    option(SERIAL "Include RS-232 driver" OFF)
    option(USB "Include USB driver" OFF)
    option(USB2 "Include USB2 driver" ON)

    # Define the libraries
    set(LIBRARIES XUP PARENT_SCOPE)
    option(XUP "Build the XW library" ON)

    # Define Miscellaneous Build Options
    set(MISC_OPTIONS ANALYSIS BUILD_EXAMPLES BUILD_TESTS VBA VLD X64 PARENT_SCOPE)
    option(ANALYSIS "Build with code analysis" OFF)
    option(EXAMPLES "Build example programs" OFF)
    option(TESTS "Build test programs" OFF)
    option(VLD "Adds VLD header support" OFF)
    option(X64 "Build for x64 platform" ON)
endfunction()

# Prints the status of the options contained in the list argument
function(validate_options name options)
    set(enabled_options "")
    foreach (option ${options})
        get_property(enabled VARIABLE PROPERTY ${option})
        if (enabled)
            list(APPEND enabled_options ${option})
        endif ()
    endforeach ()
    if ((${name} MATCHES "Devices" OR ${name} MATCHES "Protocols") AND NOT enabled_options)
        MESSAGE(FATAL_ERROR "no enabled device or protocol")
    else ()
        MESSAGE(STATUS "    ${name}: ${enabled_options}")
    endif ()

    set(ENABLED_${name} ${enabled_options} PARENT_SCOPE)
endfunction()

# A function to configure compiler options that will be used to compile any of the
# source code within the project.
function(configure_compiler_options)
    if (${CMAKE_C_COMPILER_ID} STREQUAL GNU)
        string(APPEND CMAKE_C_FLAGS " -pipe -fPIC -Wall -fcommon")
        string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
        string(APPEND CMAKE_C_FLAGS_DEBUG " -O2")
    elseif (${CMAKE_C_COMPILER_ID} STREQUAL MSVC)
        string(APPEND CMAKE_C_FLAGS " /D_CRT_SECURE_NO_WARNINGS /D_BIND_TO_CURRENT_VCLIBS_VERSION /DWIN32_LEAN_AND_MEAN /DNONAMELESSUNION")
        string(APPEND CMAKE_C_FLAGS_DEBUG " /D_DEBUG /D__MEM_DBG__")

        if (ANALYSIS)
            string(APPEND CMAKE_C_FLAGS_DEBUG " /analyze")
        endif ()

        string(REGEX REPLACE "/W3" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

        string(REGEX REPLACE "/Ob2" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        string(REGEX REPLACE "/Od" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REGEX REPLACE "/Ob0" "/O2" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        string(REGEX REPLACE "/MT" "/MD" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        string(REGEX REPLACE "/MTd" "/MDd" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        string(REGEX REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        string(REGEX REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    endif ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" PARENT_SCOPE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" PARENT_SCOPE)
endfunction()

# Handel has a number of preprocessor options that control various aspects of the build.
# These options are not the same for every subcomponent. We break the up into groups
# to make it easier to use them later as part of a `target_compile_definitions` call.
function(configure_handel_options)
    set(GENERAL_COMPILE_DEFS "")
    if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
        list(APPEND GENERAL_COMPILE_DEFS OS_LINUX=OS_LINUX)
    else ()
        list(APPEND GENERAL_COMPILE_DEFS OS_WINDOWS=OS_WINDOWS)
    endif ()

    set(DEVICE_EXCLUSIONS "")
    foreach (DEVICE ${DEVICES})
        get_property(DEVICE_ENABLED VARIABLE PROPERTY ${DEVICE})
        if (NOT DEVICE_ENABLED)
            list(APPEND DEVICE_EXCLUSIONS EXCLUDE_${DEVICE})
        else ()
            # Special handling for Ultra-Lo 1800 since it operates on inclusion.
            if (DEVICE MATCHES "ULTRALO")
                list(APPEND GENERAL_COMPILE_DEFS XIA_ALPHA)
            endif ()
        endif ()
    endforeach ()

    set(PROTOCOL_EXCLUSIONS "")
    foreach (PROTOCOL ${PROTOCOLS})
        get_property(PROTOCOL_ENABLED VARIABLE PROPERTY ${PROTOCOL})
        if (NOT PROTOCOL_ENABLED)
            list(APPEND PROTOCOL_EXCLUSIONS EXCLUDE_${PROTOCOL})
        endif ()
    endforeach ()

    set(LIBRARY_EXCLUSIONS "")
    foreach (LIBRARY ${LIBRARIES})
        get_property(LIBRARY_ENABLED VARIABLE PROPERTY ${LIBRARY})
        if (NOT LIBRARY_ENABLED)
            list(APPEND LIBRARY_EXCLUSIONS EXCLUDE_${LIBRARY})
        endif ()
    endforeach ()

    set(GENERAL_COMPILE_DEFS "${GENERAL_COMPILE_DEFS}" PARENT_SCOPE)
    set(DEVICE_EXCLUSIONS "${DEVICE_EXCLUSIONS}" PARENT_SCOPE)
    set(PROTOCOL_EXCLUSIONS "${PROTOCOL_EXCLUSIONS}" PARENT_SCOPE)
    set(LIBRARY_EXCLUSIONS "${LIBRARY_EXCLUSIONS}" PARENT_SCOPE)
endfunction()