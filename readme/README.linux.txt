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

# python
sudo apt-get install python2.7 python2.7-dev
cd /tmp && wget https://bootstrap.pypa.io/ez_setup.py -O - | sudo python
sudo easy_install pyyaml

# scons--recent versions like 2.4.0 and 2.3.6 work fine
wget http://prdownloads.sourceforge.net/scons/scons-2.4.0.tar.gz
tar xvzf scons-2.4.0.tar.gz
pushd scons-2.4.0
sudo python setup.py install --standard-lib
popd
export SCONS_DIR=/usr/local/lib/python2.7/dist-packages/SCons
alias hammer='/path/to/swtoolkit/hammer.sh'

# Handel dev dependencies
export TMP=/tmp # Handel's scons script reads this var?

# libusb
sudo apt-get install libusb-dev

# optional: Handel's test suite requires Ruby
sudo apt-get install ruby ruby-dev
sudo gem install ffi


Building with scons
===================

Handel has many flags that map to defines in the compilation environment. A
little experimentation may be needed to get the set that supports only the
products you need.

Here's an example for microDXP USB2:

  hammer --udxp --no-udxps --no-xw --no-xmap --no-stj --no-mercury --no-saturn --verbose

To search for other flags, search "SetBitFromOption" in main.scons.


Defines
=======

To integrate Handel with your own build system, you need to pass the defines
equivalent to the above scons command line. Note that not every define is
needed for every file in the source distribution.

  LINUX, HANDEL_MAKE_DLL, HANDEL_USE_DLL, EXCLUDE_UDXPS, EXCLUDE_XUP,
  EXCLUDE_XMAP, EXCLUDE_STJ, EXCLUDE_MERCURY, EXCLUDE_SATURN, EXCLUDE_SERIAL,
  EXCLUDE_EPP, EXCLUDE_PLX

If you bump into errors that seem related to a protocol or product that you're
not interested in, scan up the code for an appropriate "#ifndef EXCLUDE_" and
add the define.
