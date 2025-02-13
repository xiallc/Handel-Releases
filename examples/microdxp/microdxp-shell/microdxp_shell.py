""" SPDX-License-Identifier: Apache-2.0 """

import argparse
import cmd
import ctypes
import datetime
import json
import logging
import os
import platform
import pprint
import time

import microdxp
import utils

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

""" microdxp_shell.py
Sample code that demonstrates microDXP functionality using an interactive python shell.
"""

logging.basicConfig(format=('[%(asctime)s] %(levelname)s-%(funcName)s-%(message)s'),
                    level=logging.INFO)


def run_data_to_json(mca, stats):
    """
    Takes MCA and stats data, stuffs them into a dictionary, and returns it as a JSON
    string.

    :param mca: A ctypes array containing the MCA data.
    :param stats: An instance of the microdxp McaStats class.
    :return: A JSON string containing the current timestamp, stats, and mca.
    """
    return json.dumps({
        "timestamp": utils.get_timestamp(),
        **(utils.cstruct_as_dict(stats)),
        "mca": [x for x in mca]
    })


class MicroDxpShell(cmd.Cmd):
    intro = """Welcome to the microDXP shell prototype!
    Please be aware that this is for demonstration purposes only!
    The commands, functionality, and features may change without warning.

    Note:
    The prototype assumes you have one detector int your init file, and it's
    detector number 0.
    """
    prompt = 'microDXP> '
    file = None
    board_info = None
    serial_num = None
    current_led_status = None

    def emptyline(self):
        pass

    def default(self, line):
        self.do_help("")

    def __init__(self, handel, usb, cfg):
        super().__init__()
        self.handel = handel
        self.usb = usb
        self.cfg = cfg

        self.board_info = microdxp.BoardInformation()
        rc = self.handel.xiaBoardOperation(ctypes.c_uint(0),
                                           ctypes.c_char_p("get_board_info".encode()),
                                           ctypes.byref(self.board_info))
        if rc != 0:
            raise ValueError(f"get_board_info failed with {rc}")
        self.serial_num = microdxp.SerialNumber()
        self.handel.xiaBoardOperation(ctypes.c_uint(0),
                                      ctypes.c_char_p("get_serial_number".encode()),
                                      ctypes.byref(self.serial_num))
        if rc != 0:
            raise ValueError(f"get_board_info failed with {rc}")

    def check_code(self, name, rc):
        """
        Checks that a handel return code is 0. Raises a ValueError if not.
        """
        if rc != 0:
            raise ValueError(f"{name} failed with {rc}")

    def do_monitor(self, arg):
        """
        Monitors the temperature and HV every X seconds where x is given by the arg.
        Default polling period is 15 seconds.
        """
        period = 15.
        if arg != "":
            period = int(arg)
        starttime = time.monotonic()
        try:
            print(f"polls every {period} seconds\nCTRL+C to exit polling.")
            while True:
                print(datetime.datetime.now().isoformat())
                self.do_read("hv")
                self.do_read("temp")
                time.sleep(period - ((time.monotonic() - starttime) % period))
        except KeyboardInterrupt:
            pass

    def do_read(self, arg):
        """
        Read the requested information and print it to the screen.
        Known arguments:
           * acq <name>: Executes a xiaGetAcquisitionValues call with the provided name
           * board: prints the board information to the terminal
           * hv: reads the HV output from the I2C
           * led: reads the LED information from the I2C
           * sn: prints the board information to the terminal
           * temp: reads the on-board temperature sensor from the I2C
        """
        if arg == "":
            print("command requires an argument")
            return

        args = arg.split()

        try:
            match args[0].lower():
                case 'acq':
                    if len(args) < 2:
                        print('must include acquisition parameter')
                        return

                    # Alias set_voltage to high_voltage so we use the right acq val.
                    if args[1] == 'set_voltage':
                        args[1] = 'high_voltage'

                    detchan = ctypes.c_int(0)
                    name = ctypes.c_char_p(args[1].encode())
                    val = ctypes.c_double(0)
                    rc = self.handel.xiaGetAcquisitionValues(detchan,
                                                             name,
                                                             ctypes.byref(val))
                    if rc != 0:
                        raise ValueError(f"read {args[1]} failed with {rc}")

                    # Handel API mishandles this DSP parameter, we fix it here.
                    if args[1] == 'high_voltage':
                        args[1] = 'set_voltage'
                        val.value = val.value*self.cfg.getfloat('udxp.hv', 'set')

                    print(f"{args[1]}: {round(val.value, 3)}")
                case 'board':
                    pprint.pprint(utils.cstruct_as_dict(self.board_info))
                case 'hv':
                    cmd = microdxp.UsbSerialCommand(self.usb, 0x40)
                    cmd.append(4, int)
                    cmd.append(0, bytes)
                    cmd.append(0x29, bytes)
                    cmd.append(0, bytes)
                    cmd.append(2, bytes)

                    # The ADC is a low power device. We need to read it
                    # once to awaken it, then a second time to get the data.
                    response_length = 8
                    cmd.write()
                    time.sleep(0.1)
                    cmd.write()
                    payload = cmd.read(response_length)
                    print("monitor_voltage:",
                          round(self.cfg.getfloat('udxp.hv', 'monitor') *
                                int.from_bytes(payload[1:], 'big'), 3))
                case 'led':
                    cmd = microdxp.UsbSerialCommand(self.usb, 0xC0)
                    cmd.append(1, int)
                    cmd.append(1, bytes)
                    cmd.finalize()
                    cmd.write()
                    payload = cmd.read(11)
                    self.current_led_status = {
                        "cmd_status": payload[0],
                        "enabled": bool(payload[1]),
                        "period_us": round(
                            int.from_bytes(payload[2:4], 'little') *
                            self.cfg.getfloat('udxp.led.period', 'scale') +
                            self.cfg.getfloat('udxp.led.period', 'offset'), 2),
                        "width_ns": round(
                            int.from_bytes(payload[4:], 'little') *
                            self.cfg.getfloat('udxp.led.width', 'scale') +
                            self.cfg.getfloat('udxp.led.width', 'offset'), 2),
                    }
                    pprint.pprint(self.current_led_status)
                case 'sn':
                    pprint.pprint(utils.cstruct_as_dict(self.serial_num))
                case 'temp':
                    cmd = microdxp.UsbSerialCommand(self.usb, 0x41)
                    cmd.append(0, int)
                    cmd.write()
                    data = cmd.read(8)
                    temp = data[1]
                    for i in range(4, 8):
                        if data[2] & (1 << i):
                            temp = temp + 2 ** (i - 8)
                    print(f'temp (C): {temp}')
                case _:
                    print("unknown option")
                    self.do_help("read")
        except Exception as e:
            print(e)

    def do_run(self, arg):
        """
        Executes data runs with various options:
            * mca <type> <value> <resume>: Executes a bound MCA run.
                * type -> The type of data run to execute.
                    * real -> runs until reaching the specified clock time (default)
                    * live -> runs until reaching the specified filter live time
                    * event -> runs until reaching the specified number of events
                    * trigger -> runs until reaching the specified number of triggers
                * value -> defaults to 1
                * resume -> 0 (default) clears MCA before starting.
                            1 resumes the previous run adding to the statistics.
            * snapshot <period> <clear>: Executes a fresh, indefinite MCA run.
                * period -> Period between snapshots in seconds. Default 5 seconds.
                * clear -> 0 does not clear data on read.
                           1 (default) Clears data and stats on read.
            * trace <period> : Collects an ADC trace and returns it
                * period -> Time between ADC samples in nanoseconds. Default 25. Max 5000.
        """
        if arg == "":
            print("command requires an argument")
            return
        args = arg.split()

        try:
            match args[0].lower():
                case 'mca':
                    if len(args) != 1 and len(args) != 4:
                        print('must include run type, value and clear')
                        return

                    run_type = 1.
                    run_len = 1.
                    resume = 0
                    if len(args) == 4:
                        run_type = microdxp.PRESET_RUN_TYPES.get(args[1], 1.)
                        run_len = float(args[2])
                        resume = int(args[3])

                    self.do_set(f'acq preset_type {run_type}')
                    self.do_set(f'acq preset_value {run_len}')

                    print('starting run. CTRL+C to stop early.')
                    rc = self.handel.xiaStartRun(0, resume)
                    self.check_code("xiaStartRun", rc)

                    try:
                        run_active = ctypes.c_ushort(1)
                        remaining_time = run_len
                        while run_active.value == 1:
                            if run_type in (1, 2):
                                print(f"remaining time: {remaining_time}")
                            rc = self.handel.xiaGetRunData(0, ctypes.c_char_p(
                                "run_active".encode()), ctypes.byref(run_active))
                            self.check_code("xiaGetRunData:run_active", rc)
                            remaining_time -= 1
                            time.sleep(1)
                    except KeyboardInterrupt:
                        pass
                    finally:
                        print("stopping run")
                        rc = self.handel.xiaStopRun(0)
                        self.check_code("xiaStopRun", rc)

                    mca_len = ctypes.c_uint(0)
                    rc = self.handel.xiaGetRunData(0, ctypes.c_char_p(
                        "mca_length".encode()), ctypes.byref(mca_len))
                    self.check_code("get mca_length", rc)

                    mca = (ctypes.c_uint * mca_len.value)()
                    rc = self.handel.xiaGetRunData(0, ctypes.c_char_p("mca".encode()),
                                                   ctypes.byref(mca))
                    self.check_code("get mca", rc)

                    stats = microdxp.McaStats()
                    rc = self.handel.xiaGetRunData(0, ctypes.c_char_p(
                        "module_statistics_2".encode()), ctypes.byref(stats))
                    self.check_code("get module_statistics_2", rc)

                    print(run_data_to_json(mca, stats))
                case 'snapshot':
                    if len(args) != 1 and len(args) != 3:
                        print('must include read period and clear')
                        return

                    period = 5.
                    clear = (ctypes.c_double * 1)(*[1])
                    if len(args) == 3:
                        if float(args[1]) >= 1.:
                            period = float(args[1])
                        if not float(args[2]) in [0., 1.]:
                            raise ValueError(f"clear must be 0 or 1.")
                        else:
                            clear[0] = float(args[2])

                    self.do_set(
                        f'acq number_mca_channels {microdxp.MAX_MCA_SNAPSHOT_SIZE}')
                    self.do_set('acq preset_type 0')

                    starttime = time.monotonic()
                    rc = self.handel.xiaStartRun(0, 0)
                    self.check_code("xiaStartRun", rc)

                    mca = (ctypes.c_uint * microdxp.MAX_MCA_SNAPSHOT_SIZE)()
                    try:
                        print(f"polls every {period} seconds\nCTRL+C to exit polling.")
                        while True:
                            time.sleep(
                                period - ((time.monotonic() - starttime) % period))
                            rc = self.handel.xiaDoSpecialRun(0, ctypes.c_char_p(
                                "snapshot".encode()), ctypes.byref(clear))
                            self.check_code("snapshot", rc)

                            rc = self.handel.xiaGetSpecialRunData(0, ctypes.c_char_p(
                                "snapshot_mca".encode()), ctypes.byref(mca))
                            self.check_code("snapshot_mca", rc)

                            stats = microdxp.McaStats()
                            rc = self.handel.xiaGetSpecialRunData(0, ctypes.c_char_p(
                                "snapshot_statistics".encode()), ctypes.byref(stats))
                            self.check_code("snapshot_statistics", rc)

                            print(run_data_to_json(mca, stats))
                    except KeyboardInterrupt:
                        pass
                    finally:
                        rc = self.handel.xiaStopRun(0)
                        self.check_code("xiaStopRun", rc)
                case 'trace':
                    info = (ctypes.c_double * 2)(*[0., 25.])
                    if len(args) == 2:
                        if 25. <= float(args[1]) <= 5000.:
                            info[1] = float(args[1])

                    rc = self.handel.xiaDoSpecialRun(0, ctypes.c_char_p(
                        "adc_trace".encode()), ctypes.byref(info))
                    self.check_code("adc_trace", rc)

                    trace_len = ctypes.c_uint(0)
                    rc = self.handel.xiaGetSpecialRunData(0, ctypes.c_char_p(
                        "adc_trace_length".encode()), ctypes.byref(trace_len))
                    self.check_code("adc_trace_length", rc)

                    trace = (ctypes.c_uint * trace_len.value)()
                    rc = self.handel.xiaGetSpecialRunData(0, ctypes.c_char_p(
                        "adc_trace".encode()), ctypes.byref(trace))
                    self.check_code("adc_trace", rc)

                    print(json.dumps({"timestamp": utils.get_timestamp(),
                                      "trace": [val for val in trace]}))
                case _:
                    print("unknown option")
                    self.do_help("run")
        except Exception as e:
            print(repr(e))

    def do_set(self, arg):
        """
        Sets the requested value. Known keys:
            * acq <name> <value>: calls xiaSetAcquisitionValues with the provided name and value
                * name -> The name of the parameter that will be set
                * value -> the value that will be set
            * hv <value>: Sets the voltage to the specified value
                * value -> the voltage in units of volts
            * led <enable> <period> <width>: Sets the LED to the provided values
                * enable -> 0 = disable; 1 = enable
                * period -> LED pulse period in us
                * width -> LED pulse width in ns
        """
        if arg == "":
            print("command requires an argument")
            return
        args = arg.split()
        try:
            match args[0].lower():
                case 'acq':
                    if len(args) < 3:
                        print('must include name and value')
                        return

                    detchan = ctypes.c_int(0)
                    name = ctypes.c_char_p(args[1].encode())
                    val = ctypes.c_double(float(args[2]))
                    rc = self.handel.xiaSetAcquisitionValues(detchan,
                                                             name,
                                                             ctypes.byref(val))

                    if rc != 0:
                        raise ValueError(f"read {args[1]} failed with {rc}")

                    self.do_read(f'acq {args[1]}')
                case 'hv':
                    if len(args) < 2:
                        print('must include value')
                        return

                    val = float(args[1])
                    val_range = (self.cfg.getfloat('udxp.hv', 'low'),
                                 self.cfg.getfloat('udxp.hv', 'high'))
                    if not val_range[0] <= val < val_range[1] and val != 0.0:
                        print(f'val not in range {val_range}')
                        return
                    self.do_set(f'acq high_voltage '
                                f'{val / self.cfg.getfloat("udxp.hv", "set")}')
                case 'led':
                    if len(args) < 4:
                        print('must include all 3 values to set')
                        return

                    enable = int(args[1])
                    period = float(args[2])
                    width = float(args[3])
                    width_range = (self.cfg.getfloat('udxp.led.width', 'low'),
                                   self.cfg.getfloat('udxp.led.width', 'high'))
                    period_range = (self.cfg.getfloat('udxp.led.period', 'low'),
                                    self.cfg.getfloat('udxp.led.period', 'high'))

                    if not 0 <= enable < 2:
                        print(f"invalid enable option")
                        return
                    if not period_range[0] <= period <= period_range[1]:
                        print(
                            f"invalid led period: {period_range} "
                            f"{self.cfg.get('udxp.led.period', 'unit')}")
                        return
                    if not width_range[0] <= width <= width_range[1]:
                        print(f"invalid led width: {width_range} "
                              f"{self.cfg.get('udxp.led.width', 'unit')}")
                        return

                    period_cfg = (self.cfg.getfloat('udxp.led.period', 'offset'),
                                  self.cfg.getfloat('udxp.led.period', 'scale'))
                    width_cfg = (self.cfg.getfloat('udxp.led.width', 'offset'),
                                 self.cfg.getfloat('udxp.led.width', 'scale'))

                    cmd = microdxp.UsbSerialCommand(self.usb, 0xC0)
                    cmd.append(6, int)
                    cmd.append(0, bytes)
                    cmd.append(enable, bytes)
                    cmd.append(round((period - period_cfg[0]) / period_cfg[1]), int)
                    cmd.append(round((width - width_cfg[0]) / width_cfg[1]), int)
                    cmd.write()

                    payload = cmd.read(11)
                    self.current_led_status = {
                        "cmd_status": payload[0],
                        "enabled": bool(payload[1]),
                        "period_us": round(int.from_bytes(payload[2:4], 'little') *
                                           period_cfg[1] + period_cfg[0], 2),
                        "width_ns": round(int.from_bytes(payload[4:], 'little') *
                                          width_cfg[1] + width_cfg[0], 2),
                    }
                    pprint.pprint(self.current_led_status)
                case _:
                    print("unknown option")
                    self.do_help("set")
        except Exception as e:
            print(repr(e))

    def do_toggle_led(self, arg):
        """
        Toggles the LED on or off.
        """
        try:
            self.do_read('led')
            self.do_set(f"led {int(not self.current_led_status['enabled'])} "
                        f"{self.current_led_status['period_us']} "
                        f"{self.current_led_status['width_ns']}")
        except Exception as e:
            print(e)

    def close(self):
        if self.file:
            self.file.close()
            self.file = None

    def do_exit(self, arg):
        """
        Exit the shell.
        """
        self.close()
        return True


def load_library(path):
    """
    The path to the library that needs to be loaded. We'll take different actions
    depending on the specific library that we need.
    :param path: The relative or absolute path to the library that we will load.
    :return: The loaded library.
    """
    if os.path.isdir(path):
        raise ValueError("path should be a file")
    if not os.path.isfile(path):
        raise FileNotFoundError(f"library does not exist: {path}")
    logging.info(f'loading {path}')
    return ctypes.cdll.LoadLibrary(path)


def init(handel, ini):
    """
    Initializes Handel and starts the system
    :param handel: the handel library object
    :param ini: The path to the .ini file that we should use.
    :return: The status code from Handel
    """
    if not os.path.exists(ini):
        raise FileNotFoundError(f'cannot find {ini} ')

    logging.info(f"config: {ini}")
    ini_file = ctypes.c_char_p(ini.encode())

    handel.xiaSetLogLevel(4)
    handel.xiaSetLogOutput(ctypes.c_char_p("handel.log".encode()))

    rc = handel.xiaInit(ini_file)
    if rc != 0:
        raise ConnectionError(f'handel init failed with error {rc}')

    rc = handel.xiaStartSystem()
    if rc != 0:
        raise ConnectionError(f'handel start failed with error {rc}')


def main(args):
    handel = None
    try:
        cfg = utils.read_ini(args.ini)

        if platform.system() == 'Windows':
            handel_lib = "handel.dll"
            usb_lib = "xia_usb2.dll"
        elif platform.system() == 'Linux':
            handel_lib = "libhandel.so"
            usb_lib = "libxia_usb2.so"
        else:
            raise NotImplementedError(f"Unsupported OS {platform.system()}")

        handel = load_library(os.path.join(args.lib, handel_lib))
        usb = load_library(os.path.join(args.lib, usb_lib))
        init(handel, args.ini)
        MicroDxpShell(handel, usb, cfg).cmdloop()
        rc = handel.xiaExit()
        if rc != 0:
            return False
        rc = handel.xiaCloseLog()
        if rc != 0:
            return False
    except Exception as e:
        logging.error(e)


if __name__ == '__main__':
    try:
        parser = argparse.ArgumentParser(
            description='Provides an example of using Handel with python.')
        parser.add_argument('-l', '--lib',
                            help='Provides the path to the handel library directory.',
                            default="./libs/1.2.30/lib")
        parser.add_argument('-i', '--ini',
                            help='Provides the path to the ini file.',
                            default="./ini/template.ini")
        args = parser.parse_args()
        main(args)
    except KeyboardInterrupt:
        logging.info("Received keyboard interrupt. Stopping execution.")
    finally:
        logging.info("Finished execution.")
