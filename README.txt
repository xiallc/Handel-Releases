Handel 1.1 is built with SCons+swtoolkit. The version
requirements of particular dependencies are not all strict, but the
more swtoolkit ages, the more likely we are to get stuck on a version
of SCons. When in doubt, use the version listed.

1. Install the latest Python27 Windows installer
   https://www.python.org/downloads/. The x86-64 version is fine on a 64-bit
   system.
2. Install pywin32 to match your Python installation. For a 64-bit
   Python, select pywin32-220.win-amd64-py2.7.exe from
   https://sourceforge.net/projects/pywin32/files/pywin32/Build%20220/.
3. Get SCons 2.4.0. If the Windows installer doesn't work (can't see a
   registered Python version), install manually by downloading the zip and
   running =python setup.py install --standard-lib=.
4. For hammer.bat (swtoolkit's main script, included in Handel) to work, =set
   SCONS_DIR=c:\python27\lib\site-packages\scons=.
5. Install pyyaml: =c:\python27\scripts\easy_install pyyaml=.

Tests in t_api run with Ruby 1.9+. You will need both x86 and x64
versions of Ruby in order to test both builds of Handel. For each
version, `gem install ffi`.

tools/releaser requires more Ruby gems, to be enumerated.

# Working with SCons

Note: from the source distribution, move src/main.scons to the root of the
package.

`main.scons` sets up all the product flags and includes subdirectory scons
files. Because we use swtoolkit, our scons files contain a mix of swtoolkit
functions and scons functions.

Resources:

* [swtoolkit examples](https://code.google.com/p/swtoolkit/wiki/Examples)
* [swtoolkit glossary](https://code.google.com/p/swtoolkit/wiki/Glossary#COMPONENT_LIBRARY_PUBLISH)
* [SCons manual](http://www.scons.org/doc/production/HTML/scons-man.html)
