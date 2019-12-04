XIA builds Handel with scons (stock) + swtoolkit (slightly modified).

The scons build makes heavy use of static libraries and produces one shared
library for the Handel API and a shared library for each driver protocol (e.g.
xia_usb2 and xia_plx).

This document lists the steps to get Handel building on Linux with those tools
and the default structure. However, you can also skip scons and integrate with
your own build system with any library structure you like.


Makefile reference
==================

For a non-scons reference, here is a sample Makefile from a Handel user that
sets flags and source lists for a previous version of Handel:
https://subversion.xray.aps.anl.gov/synApps/dxp/trunk/dxpApp/handelSrc/Makefile


scons dev setup
===============

Note: from the source distribution, move src/main.scons to the root of the
package.

# Python
sudo apt-get install python2.7 python2.7-dev
sudo pip install pyyaml

# scons 2.x. It may work installed from pip on some systems, but
# we have to install from source to get the paths to work.
wget https://github.com/SCons/scons/archive/2.5.0.tar.gz
tar xvzf scons-2.5.0.tar.gz
pushd scons-2.5.0
sudo python setup.py install --standard-lib
popd
export SCONS_DIR=/usr/local/lib/python2.7/dist-packages/SCons
alias hammer='/path/to/swtoolkit/hammer.sh'

# Handel dev dependencies
export TMP=/tmp # Handel's scons script reads this var?

# optional: Handel's test suite requires Ruby
sudo apt-get install ruby ruby-dev
sudo gem install ffi


Building with scons
===================

Handel has many flags that map to defines in the compilation environment. A
little experimentation may be needed to get the set that supports only the
products you need.

Here's an example for microDXP USB2:

   hammer --udxp --no-udxps --no-xw --no-serial --no-xmap --no-stj --no-mercury --no-saturn --verbose

To search for other flags, search "SetBitFromOption" in main.scons.


Defines
=======

This section lists defines you can use in your build system to control
the features compiled in Handel. Note that not every define is needed
for every file in the source distribution.

Disable products and protocols not currently supported on Linux:

    EXCLUDE_XMAP EXCLUDE_STJ EXCLUDE_PLX

Selectively disable products you do not use:

    EXCLUDE_MERCURY EXCLUDE_SATURN EXCLUDE_UDXP

When building for microDXP, always define these internal feature
exclusions, which are not available in the customer release:

    EXCLUDE_UDXPS EXCLUDE_XUP

Selectively disable interface protocols you do not use:

    EXCLUDE_SERIAL EXCLUDE_EPP EXCLUDE_USB EXCLUDE_USB2


USB Setup
=========

Handel USB and USB2 support on Linux are built on libusb-0.1.

  sudo apt-get install libusb-dev

Check that libusb sees the XIA device:

  lsusb -v | grep "ID: 10e9"

By default, root is required to open the device. This snippet shows
how to set up Ubuntu udev rules to enable user access:

```
# Confirm your user is in the plugdev group
groups

# Set up udev rules. Customize idProduct for your device.
sudo cat > /etc/udev/rules.d/99-xia-usb.rules <<RULES
ACTION=="add", SUBSYSTEMS=="usb", ATTRS{idVendor}=="10e9", ATTRS{idProduct}=="0702", MODE="660", GROUP="plugdev"
ACTION=="add", SUBSYSTEMS=="usb", ATTRS{idVendor}=="10e9", ATTRS{idProduct}=="0b01", MODE="660", GROUP="plugdev"
RULES
```

It may be helpful in debugging to monitor USB transfers. This is easy
if your system includes usbmon:

```
mount -t debugfs none_debugs /sys/kernel/debug
modprobe usbmon

cat /sys/kernel/debug/usb/usbmon/1u | sed -n -e "s/^[a-f0-9]\+ [0-9]\+//p"
```


Serial Port Setup
=================

Handel v1.2.19 and later includes a serial port implementation built
on termios. This interface is experimental but does pass all microDXP
tests in XIA's testing. To use it, edit the module definition in your
.ini file to set the interface to "serial". Instead of defining
com_port as you would for the Windows serial interface, locate the
device in /dev and put this file in the device\_file setting. Here is
a full sample module definition:

```
[module definitions]
START #0
alias = module1
module_type = udxp
interface = serial
device_file = /dev/ttyS1
baud_rate = 230400
number_of_channels = 1
channel0_alias = 0
channel0_detector = detector1:0
firmware_set_chan0 = firmware1
default_chan0 = defaults_module1_0
END #0
```
