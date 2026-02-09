""" SPDX-License-Identifier: Apache-2.0 """
import ctypes

"""
Copyright 2024 XIA LLC, All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

""" microdxp.py

"""

MAX_MCA_SNAPSHOT_SIZE = 4096

PRESET_RUN_TYPES = {
    'none': 0.,
    'real': 1.,
    'live': 2.,
    'event': 3.,
    'trigger': 4.
}


class LedPayload(ctypes.Structure):
    _fields_ = [
        ("cmd_status", ctypes.c_char),
        ("enable", ctypes.c_char),
        ("period", ctypes.c_char * 2),
        ("width", ctypes.c_char * 2),
    ]


class McaStats(ctypes.Structure):
    """
    MCA statistics object obtained from the module_statistics_2 run data.
    """
    _fields_ = [
        ("run_time", ctypes.c_double),
        ("trigger_livetime", ctypes.c_double),
        ("energy_livetime", ctypes.c_double),
        ("triggers", ctypes.c_double),
        ("events", ctypes.c_double),
        ("icr", ctypes.c_double),
        ("ocr", ctypes.c_double),
        ("underflows", ctypes.c_double),
        ("overflows", ctypes.c_double)
    ]


class SerialNumber(ctypes.Structure):
    """
    A class defining the information recorded in the microDXP's serial number data.
    """
    _fields_ = [("header", ctypes.c_char * 3),
                ("variant", ctypes.c_char * 2),
                ("revision", ctypes.c_char * 2),
                ("batch_week", ctypes.c_char * 2),
                ("batch_year", ctypes.c_char * 2),
                ("sn", ctypes.c_char * 6)]


class BoardInformation(ctypes.Structure):
    """
    A class containing the information recorded in the microDXP's board information.
    Note that we do not decode the FIPPI information.
    """
    _fields_ = [("pic_variant", ctypes.c_ubyte),
                ("pic_major", ctypes.c_ubyte),
                ("pic_minor", ctypes.c_ubyte),
                ("dsp_variant", ctypes.c_ubyte),
                ("dsp_major", ctypes.c_ubyte),
                ("dsp_minor", ctypes.c_ubyte),
                ("clock_speed_mhz", ctypes.c_ubyte),
                ("clock_enable", ctypes.c_ubyte),
                ("fippi_count", ctypes.c_ubyte),
                ("gain_mode", ctypes.c_ubyte),
                ("gain_low", ctypes.c_ubyte),
                ("gain_high", ctypes.c_ubyte),
                ("gain_exponent", ctypes.c_ubyte),
                ("nyquist_filter", ctypes.c_ubyte),
                ("adc_speed_grade", ctypes.c_ubyte),
                ("fpga_speed_grade", ctypes.c_ubyte),
                ("analog_power", ctypes.c_ubyte),
                ("fippi0.decimation", ctypes.c_ubyte),
                ("fippi0.version", ctypes.c_ubyte),
                ("fippi0.variant", ctypes.c_ubyte),
                ("fippi1.decimation", ctypes.c_ubyte),
                ("fippi1.version", ctypes.c_ubyte),
                ("fippi1.variant", ctypes.c_ubyte),
                ("fippi2.decimation", ctypes.c_ubyte),
                ("fippi2.version", ctypes.c_ubyte),
                ("fippi2.variant", ctypes.c_ubyte)]
