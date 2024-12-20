# microDXP Shell

The microDXP Shell provides a prototype interactive shell to interact with the microDXP
hardware. The shell allows users to read and set data, monitor system information,
control HV and LED pulsers. For a full list of commands use the shell's `help` command.

This project provides an example of how to work with a microDXP device through the
Handel API using Python. While we strive to ensure that our code is free from
errors, please be aware that this is still in a pre-release state. Functionality,
commands, and features may change without warning.

## Requirements
1. Python 3.12+
2. [Handel 1.2.30](https://github.com/xiallc/Handel-Releases/releases/tag/1.2.30)
3. A handel INI file (template provided in this project)

### Starting the shell
The shell is invoked using python to execute `microdxp_shell.py`. For example,
`python ./microdxp_shell.py`. We have tried to limit library usage to the python
standard library. No additional packages should be required.

```
usage: microdxp_shell.py [-h] [-l LIB] [-i INI]

Provides an example of using Handel with python.

options:
  -h, --help         show this help message and exit
  -l LIB, --lib LIB  Provides the path to the handel libraries.
  -i INI, --ini INI  Provides the path to the ini file.
```

