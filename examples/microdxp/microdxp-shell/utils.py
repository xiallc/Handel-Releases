""" SPDX-License-Identifier: Apache-2.0 """
import configparser

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

""" utils.py

"""


def cstruct_as_dict(struct):
    """
    Processes a cstruct and returns it as a dictionary.
    """
    return dict((f, getattr(struct, f)) for f, _ in struct._fields_)


def read_ini(file):
    """
    Reads the Handel ini file, validates the shell sections, and returns the config.
    """
    parser = configparser.ConfigParser(allow_no_value=True)
    parser.read(file)

    for section in parser.sections():
        if section in ('udxp.hv', 'udxp.led.width', 'udxp.led.period'):
            if (parser.getfloat(section, 'low') >
                parser.getfloat(section, 'high')):
                raise ValueError(f'{section}: low > high')

    return parser
