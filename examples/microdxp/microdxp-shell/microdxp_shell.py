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


def run_data_to_json(detnum, mca, stats):
    """
    Takes MCA and stats data, stuffs them into a dictionary, and returns it as a JSON
    string.

    :param mca: A ctypes array containing the MCA data.
    :param stats: An instance of the microdxp McaStats class.
    :return: A JSON string containing the current timestamp, stats, and mca.
    """
    return json.dumps({
        "timestamp": utils.get_timestamp(),
        "detnum": detnum,
        **(utils.cstruct_as_dict(stats)),
        "mca": [x for x in mca]
    })


def check_code(name, rc):
    """
    Checks that a handel return code is 0. Raises a ValueError if not.
    """
    if rc != 0:
        raise ValueError(f"{name} failed with {rc}")


class MicroDxpShell(cmd.Cmd):
    modules = []
    intro = f"""Welcome to the microDXP shell prototype!
    Please be aware that this is for demonstration purposes only!
    The commands, functionality, and features may change without warning.
    """
    prompt = 'microDXP> '
    file = None

    def emptyline(self):
        pass

    def default(self, line):
        self.do_help("")

    def __init__(self, handel, cfg):
        super().__init__()
        self.handel = handel
        self.cfg = cfg

        for chan in range(cfg.getint('detector definitions', 'number_of_channels')):
            board_info = microdxp.BoardInformation()
            rc = self.handel.xiaBoardOperation(ctypes.c_uint(chan),
                                               ctypes.c_char_p("get_board_info".encode()),
                                               ctypes.byref(board_info))
            if rc != 0:
                raise ValueError(f"get_board_info failed with {rc}")

            serial_num = microdxp.SerialNumber()
            rc = self.handel.xiaBoardOperation(ctypes.c_uint(chan),
                                               ctypes.c_char_p("get_serial_number".encode()),
                                               ctypes.byref(serial_num))
            if rc != 0:
                raise ValueError(f"get_board_info failed with {rc}")

            self.modules.append({
                "info": utils.cstruct_as_dict(board_info),
                "sn": {k: v.decode() for k, v in utils.cstruct_as_dict(serial_num).items()},
                "led": {}
            })

    def do_monitor(self, arg):
        """
        Monitors the temperature and HV every X seconds where x is given by the arg. Set device_id to
        `-1` to monitor all devices.
        Usage: read device_id poll_period
        Options:
           * poll_period: Defaults to 15 seconds.
        """
        args = arg.split()
        if len(args) < 1:
            self.do_help("monitor")
            return

        period = 15.
        if len(args) == 2:
            period = int(args[1])

        device_id = int(args[0])
        device_set = device_id,
        if device_id == -1 or device_id > len(self.modules):
            device_set = range(len(self.modules))

        starttime = time.monotonic()
        try:
            print(f"polls every {period} seconds\nCTRL+C to exit polling.")
            while True:
                print(datetime.datetime.now().isoformat())
                for device in device_set:
                    self.do_read(f"{device} hv")
                    self.do_read(f"{device} temp")
                time.sleep(period - ((time.monotonic() - starttime) % period))
        except KeyboardInterrupt:
            pass

    def do_read(self, arg):
        """
        Read the requested information and print it to the screen.
        Usage: read device_id <operation>
        Known operations:
           * acq <name>: Executes a xiaGetAcquisitionValues call with the provided name
           * hv: reads the HV output from the I2C
           * info: prints the board information to the terminal
           * led: reads the LED information from the I2C
           * sn: prints the board information to the terminal
           * temp: reads the on-board temperature sensor from the I2C
        """
        args = arg.split()
        if len(args) < 2:
            self.do_help("read")
            return

        try:
            device_id = int(args[0])
            device_set = device_id,
            if device_id == -1 or device_id > len(self.modules):
                device_set = range(len(self.modules))

            opt = args[1].lower()

            for device in device_set:
                if opt == 'acq':
                    if len(args) < 3:
                        print('must include acquisition parameter')
                        return

                    # Alias set_voltage to high_voltage so we use the right acq val.
                    if args[2] == 'set_voltage':
                        args[2] = 'high_voltage'

                    name = ctypes.c_char_p(args[2].encode())
                    val = ctypes.c_double(0)
                    rc = self.handel.xiaGetAcquisitionValues(ctypes.c_int(device),
                                                             name,
                                                             ctypes.byref(val))
                    if rc != 0:
                        raise ValueError(f"read {args[2]} failed with {rc}")

                    # Handel API mishandles this DSP parameter, we fix it here.
                    if args[1] == 'high_voltage':
                        args[1] = 'set_voltage'
                        val.value = val.value * self.cfg.getfloat('udxp.hv', 'set')

                    print(f"{device}.{args[2]}: {round(val.value, 3)}")
                elif opt == 'hv':
                    cmd = microdxp.UsbSerialCommand(self.handel, device, 0x40)
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
                    print(f"{device_id}.monitor_voltage:",
                          round(self.cfg.getfloat('udxp.hv', 'monitor') *
                                int.from_bytes(payload[1:], 'big'), 3))
                elif opt == 'info':
                    print(f"{device}.info:", json.dumps(self.modules[device]['info']))
                elif opt == 'led':
                    cmd = microdxp.UsbSerialCommand(self.handel, device, 0xC0)
                    cmd.append(1, int)
                    cmd.append(1, bytes)
                    cmd.finalize()
                    cmd.write()
                    payload = cmd.read(11)
                    self.modules[device_id]['led'] = {
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
                    print(f"{device}.led:", self.modules[device]['led'])
                elif opt == 'sn':
                    print(f"{device}.sn:", json.dumps(self.modules[device]['sn']))
                elif opt == 'temp':
                    cmd = microdxp.UsbSerialCommand(self.handel, device, 0x41)
                    cmd.append(0, int)
                    cmd.write()
                    data = cmd.read(8)
                    temp = data[1]
                    for i in range(4, 8):
                        if data[2] & (1 << i):
                            temp = temp + 2 ** (i - 8)
                    print(f'{device}.temp_c: {temp}')
                else:
                    print("unknown operation")
                    self.do_help("read")
        except Exception as e:
            print(e)

    def do_run(self, arg):
        """
        Executes data runs of various types. Data runs with a device ID set to -1 will
        execute against all channels in the system.
        Usage: run device_id <type>
        Run types:
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
        args = arg.split()
        if len(args) < 2:
            self.do_help("run")
            return

        try:
            device_id = int(args[0])
            device_set = device_id,
            if device_id == -1:
                device_set = range(len(self.modules))
            opt = args[1].lower()

            if opt == 'mca':
                if len(args) != 2 and len(args) != 5:
                    print('must include run type, value and clear')
                    return

                run_type = 1.
                run_len = 1.
                resume = 0
                if len(args) == 5:
                    run_type = microdxp.PRESET_RUN_TYPES.get(args[2], 1.)
                    run_len = float(args[3])
                    resume = int(args[4])

                for device in device_set:
                    self.do_set(f'{device} acq preset_type {run_type}')
                    self.do_set(f'{device} acq preset_value {run_len}')

                print('starting run. CTRL+C to stop early.')
                rc = self.handel.xiaStartRun(device_id, resume)
                check_code("xiaStartRun", rc)

                try:
                    run_active = ctypes.c_ushort(1)
                    remaining_time = run_len
                    while run_active.value == 1:
                        if run_type in (1, 2):
                            print(f"remaining time: {remaining_time}")
                        rc = self.handel.xiaGetRunData(0, ctypes.c_char_p(
                            "run_active".encode()), ctypes.byref(run_active))
                        check_code("xiaGetRunData:run_active", rc)
                        remaining_time -= 1
                        time.sleep(1)
                except KeyboardInterrupt:
                    pass
                finally:
                    print("stopping run")
                    rc = self.handel.xiaStopRun(device_id)
                    check_code("xiaStopRun", rc)

                for device in device_set:
                    mca_len = ctypes.c_ulong(0)
                    rc = self.handel.xiaGetRunData(device, ctypes.c_char_p(
                        "mca_length".encode()), ctypes.byref(mca_len))
                    check_code("get mca_length", rc)

                    mca = (ctypes.c_ulong * mca_len.value)()
                    rc = self.handel.xiaGetRunData(device, ctypes.c_char_p("mca".encode()),
                                                   ctypes.byref(mca))
                    check_code("get mca", rc)

                    stats = microdxp.McaStats()
                    rc = self.handel.xiaGetRunData(device, ctypes.c_char_p(
                        "module_statistics_2".encode()), ctypes.byref(stats))
                    check_code("get module_statistics_2", rc)

                    print(run_data_to_json(device, mca, stats))
            elif opt == 'snapshot':
                if len(args) != 2 and len(args) != 4:
                    print('must include read period and clear')
                    return

                period = 5.
                clear = (ctypes.c_double * 1)(*[1])
                if len(args) == 4:
                    if float(args[2]) >= 1.:
                        period = float(args[1])
                    if not float(args[3]) in [0., 1.]:
                        raise ValueError(f"clear must be 0 or 1.")
                    else:
                        clear[0] = float(args[3])

                for device in device_set:
                    self.do_set(
                        f'{device} acq number_mca_channels {microdxp.MAX_MCA_SNAPSHOT_SIZE}')
                    self.do_set(f'{device} acq preset_type 0')

                starttime = time.monotonic()
                rc = self.handel.xiaStartRun(device_id, 0)
                check_code("xiaStartRun", rc)

                mca = (ctypes.c_ulong * microdxp.MAX_MCA_SNAPSHOT_SIZE)()
                try:
                    print(f"polls every {period} seconds\nCTRL+C to exit polling.")
                    while True:
                        time.sleep(
                            period - ((time.monotonic() - starttime) % period))

                        rc = self.handel.xiaDoSpecialRun(device_id, ctypes.c_char_p(
                            "snapshot".encode()), ctypes.byref(clear))
                        check_code("snapshot", rc)

                        for device in device_set:
                            rc = self.handel.xiaGetSpecialRunData(device, ctypes.c_char_p(
                                "snapshot_mca".encode()), ctypes.byref(mca))
                            check_code("snapshot_mca", rc)

                            stats = microdxp.McaStats()
                            rc = self.handel.xiaGetSpecialRunData(device, ctypes.c_char_p(
                                "snapshot_statistics".encode()), ctypes.byref(stats))
                            check_code("snapshot_statistics", rc)

                            print(run_data_to_json(device, mca, stats))
                except KeyboardInterrupt:
                    pass
                finally:
                    rc = self.handel.xiaStopRun(device_id)
                    check_code("xiaStopRun", rc)
            elif opt == 'trace':
                info = (ctypes.c_double * 2)(*[0., 25.])
                if len(args) == 3:
                    if 25. <= float(args[2]) <= 5000.:
                        info[1] = float(args[2])

                rc = self.handel.xiaDoSpecialRun(device_id, ctypes.c_char_p(
                    "adc_trace".encode()), ctypes.byref(info))
                check_code("adc_trace", rc)

                for device in device_set:
                    trace_len = ctypes.c_ulong(0)
                    rc = self.handel.xiaGetSpecialRunData(device, ctypes.c_char_p(
                        "adc_trace_length".encode()), ctypes.byref(trace_len))
                    check_code("adc_trace_length", rc)

                    trace = (ctypes.c_ulong * trace_len.value)()
                    rc = self.handel.xiaGetSpecialRunData(device, ctypes.c_char_p(
                        "adc_trace".encode()), ctypes.byref(trace))
                    check_code("adc_trace", rc)

                    print(json.dumps({"timestamp": utils.get_timestamp(),
                                      "id": device,
                                      "trace": [val for val in trace]}))
            else:
                print("unknown option")
                self.do_help("run")
        except Exception as e:
            print(repr(e))

    def do_set(self, arg):
        """
        Sets the requested value.
        Usage: set <device_id> operation
        Known operations:
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
        args = arg.split()
        if len(args) < 3:
            self.do_help("set")
            return

        try:
            device_id = int(args[0])
            device_set = device_id,
            if device_id == -1 or device_id > len(self.modules):
                device_set = range(len(self.modules))
            opt = args[1].lower()

            for device in device_set:
                if opt == 'acq':
                    if len(args) < 4:
                        print('must include name and value')
                        return

                    name = ctypes.c_char_p(args[2].encode())
                    val = ctypes.c_double(float(args[3]))
                    rc = self.handel.xiaSetAcquisitionValues(ctypes.c_int(device),
                                                             name,
                                                             ctypes.byref(val))
                    if rc != 0:
                        raise ValueError(f"read {args[2]} failed with {rc}")
                    self.do_read(f'{device} acq {args[2]}')
                elif opt == 'hv':
                    val = float(args[2])
                    val_range = (self.cfg.getfloat('udxp.hv', 'low'),
                                 self.cfg.getfloat('udxp.hv', 'high'))
                    if not val_range[0] <= val < val_range[1] and val != 0.0:
                        print(f'val not in range {val_range}')
                        return
                    self.do_set(f'{device} acq high_voltage '
                                f'{val / self.cfg.getfloat("udxp.hv", "set")}')
                elif opt == 'led':
                    if len(args) < 5:
                        print('must include all 3 values to set')
                        return

                    enable = int(args[2])
                    period = float(args[3])
                    width = float(args[4])
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

                    cmd = microdxp.UsbSerialCommand(self.handel, device, 0xC0)
                    cmd.append(6, int)
                    cmd.append(0, bytes)
                    cmd.append(enable, bytes)
                    cmd.append(round((period - period_cfg[0]) / period_cfg[1]), int)
                    cmd.append(round((width - width_cfg[0]) / width_cfg[1]), int)
                    cmd.write()

                    payload = cmd.read(11)
                    self.modules[device]['led'] = {
                        "cmd_status": payload[0],
                        "enabled": bool(payload[1]),
                        "period_us": round(int.from_bytes(payload[2:4], 'little') *
                                           period_cfg[1] + period_cfg[0], 2),
                        "width_ns": round(int.from_bytes(payload[4:], 'little') *
                                          width_cfg[1] + width_cfg[0], 2),
                    }
                    print(f"{device}.led:", json.dumps(self.modules[device]['led']))
                else:
                    print("unknown option")
                    self.do_help("set")
        except Exception as e:
            print(repr(e))

    def do_toggle_led(self, arg):
        """
        Toggles the LED on or off.
        Usage toggle_led <device_id>
        """
        args = arg.split()
        if len(args) < 1:
            self.do_help("toggle_led")
            return
        try:
            device_id = int(args[0])
            device_set = device_id,
            if device_id == -1 or device_id > len(self.modules):
                device_set = range(len(self.modules))

            for device in device_set:
                self.do_read(f'{device} led')
                self.do_set(f"{device} led "
                            f"{int(not self.modules[device]['led']['enabled'])} "
                            f"{self.modules[device]['led']['period_us']} "
                            f"{self.modules[device]['led']['width_ns']}")
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
        elif platform.system() == 'Linux':
            handel_lib = "libhandel.so"
        else:
            raise NotImplementedError(f"Unsupported OS {platform.system()}")

        handel = load_library(os.path.join(args.lib, handel_lib))
        init(handel, args.ini)
        MicroDxpShell(handel, cfg).cmdloop()
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
