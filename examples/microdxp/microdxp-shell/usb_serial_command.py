""" SPDX-License-Identifier: Apache-2.0 """
import ctypes

"""
Copyright 2026 XIA LLC, All rights reserved.

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


class UsbSerialCommand:
    """
    Defines and executes a serial command within the RS232 command spec framework.
    The command actually gets executed over the USB 2.0 interface.
    """

    def __init__(self, lib, device_id, cmd):
        """
        Initializes the variables we need
        :param lib: The handel library that handles communication with the device.
        :param device_id: The device id to send the command to.
        :param cmd: The command that we'll be executing.
        """
        self.lib = lib
        self.cmd = bytearray(cmd.to_bytes(1, 'little'))
        self.UART_ADDRESS_0 = 0x01000000

        self.device_id = device_id
        self.handle = ctypes.c_void_p(0)
        self.response = None
        self.payload = None

        self.finalized = False
        self.opened = False

    def __del__(self):
        """
        Ensures that we close when we go out of scope.
        :return:
        """
        self.close()

    def __str__(self):
        """
        :return: The command as a hex formatted string.
        """
        return bytes(self.cmd).hex(' ')

    def close(self):
        """
        Closes the
        :return:
        """
        if self.opened:
            rc = self.lib.xia_usb2_close(self.handle)
            if rc != 0:
                raise IOError(f"close usb2 device failed with {rc}")

    def append(self, val, val_type):
        """
        Appends the provided value to the end of the command
        :param val: the value that will be appended
        :param val_type: The type that we will append the value as.
        :return: None
        """
        if self.finalized:
            raise PermissionError("command already finalized")

        if val_type == bytes:
            if val > 255:
                raise ValueError(f'value ({val}) !< 255')
            self.cmd.extend(val.to_bytes(1, 'little'))
        elif val_type == int:
            self.cmd.extend(val.to_bytes(2, 'little'))
        else:
            raise TypeError(f'type {type(val)} not supported')

    def finalize(self):
        """
        Finalizes the command by computing the checksum, appending it to the command,
            and prepending the ESC character.
        :return: None
        """
        checksum = 0
        for x in self.cmd:
            checksum ^= x
        self.append(checksum, bytes)
        self.cmd[0:0] = bytearray(b'\x1b')
        self.finalized = True

    def open(self):
        """
        Opens up a new USB2 based RS232 connection for the device provided.
        :param device_id: The ID of the device to open. Defaults to 0.
        :return: None
        """
        rc = self.lib.xia_usb2_open(ctypes.c_int(self.device_id),
                                    ctypes.byref(self.handle))
        if rc != 0:
            raise IOError(f"open usb2 device failed with {rc}")
        self.opened = True

    def read(self, size):
        """
        Reads the provided number of bytes to get the response. Verifies the checksum.
        Returns the payload as an array.
        :param size: the number of bytes to read
        :return: An array containing just the data payload.
        """
        response = ctypes.create_string_buffer(size)
        rc = self.lib.xia_usb2_read(self.handle, ctypes.c_ulong(self.UART_ADDRESS_0),
                                    ctypes.c_ulong(size), ctypes.byref(response))
        if rc != 0:
            raise IOError(f"read usb2 device failed with {rc}")

        checksum = 0
        for x in response[1:len(response) - 1]:
            checksum ^= x
        if checksum != int.from_bytes(response[-1], 'little'):
            raise IOError(f"checksum mismatch {bytes(checksum).hex()} != "
                          f"{bytes(response[-1])}")

        payload = response[4: len(response) - 1]
        if len(payload) != int.from_bytes(response[2:3], byteorder='little'):
            raise ValueError("payload length mismatch")
        self.response = response
        self.payload = payload
        return self.payload

    def write(self):
        """
        Writes the command to the device.
        :return: None
        """
        if not self.finalized:
            self.finalize()
        if not self.opened:
            self.open()

        c_pkt = (ctypes.c_ubyte * len(self.cmd))(*self.cmd)
        c_pkti = ctypes.pointer(c_pkt)

        rc = self.lib.xia_usb2_write(self.handle,
                                     ctypes.c_ulong(self.UART_ADDRESS_0),
                                     ctypes.c_ulong(len(self.cmd)),
                                     c_pkti)
        if rc != 0:
            raise IOError(f"write usb2 device failed with {rc}")
