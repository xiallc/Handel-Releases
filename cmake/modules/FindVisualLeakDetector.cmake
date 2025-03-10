# SPDX-License-Identifier: Apache-2.0

# Copyright 2021 XIA LLC, All rights reserved.
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

# @file FindVisualLeakDetector.cmake
# @brief Finds the Visual Leak Detector library
#
# This module defines the following variables:
# VLD_LIBRARY_DIR - Location of the VLD libraries.
# VLD_INCLUDE_DIR - Location of the VLD headers.
# VLD_COMPILE_FLAGS - Flags used when compiling the software for VLD support.
# VLD_FOUND - TRUE if we found VLD_LIBRARY_DIR, VLD_INCLUDE_DIR, VLD_STATIC_LIBRARY_PATH

if (X64)
    set(VLD_ARCH_PATH "Win64")
else ()
    set(VLD_ARCH_PATH "Win32")
endif ()

find_path(VLD_LIBRARY_DIR
        NAMES vld.lib
        HINTS $ENV{VLD_SDK_DIR}
        PATHS "C:\\Program Files (x86)\\Visual Leak Detector\\lib"
        PATH_SUFFIXES ${VLD_ARCH_PATH})

find_path(VLD_INCLUDE_DIR
        NAMES vld.h vld_def.h
        HINTS $ENV{VLD_SDK_DIR}
        PATHS "C:\\Program Files (x86)\\Visual Leak Detector"
        PATH_SUFFIXES include
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VisualLeakDetector DEFAULT_MSG
        VLD_LIBRARY_DIR VLD_INCLUDE_DIR)

if (VisualLeakDetector_FOUND OR VISUALLEAKDETECTOR_FOUND)
    set(VLD_COMPILE_FLAGS "__VLD_MEM_DBG__")
    mark_as_advanced(VLD_LIBRARY_DIR VLD_INCLUDE_DIR VLD_COMPILE_FLAGS)
endif ()