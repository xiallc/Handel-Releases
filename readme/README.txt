Handel 1.1 is built with SCons+swtoolkit. I'm not sure it matters exactly what
versions of things you have, but here is a setup that works:

1. Install the latest Python27 Windows installer
   https://www.python.org/downloads/. The x86-64 version is fine on a 64-bit
   system.
2. Get scons 2.3.6. If the Windows installer doesn't work (can't see a
   registered Python version), install manually by downloading the zip and
   running =python setup.py install --standard-lib=.
3. For hammer.bat (swtoolkit's main script, included in Handel) to work, =set
   SCONS_DIR=c:\python27\lib\site-packages\scons=.
4. Install pyyaml: =c:\python27\scripts\easy_install pyyaml=.

Tests in t_api run with Ruby. You will need both x86 and x64 Ruby versions in
order to test both builds of Handel. 1.9.3 was our standard for a while, but
2.0-x64 is required to test the x64 build. Fortunately Ruby 2.0 seems to run
our tests with no problems. For each version, you need the FFI gem: =gem
install ffi=.

tools/releaser probably requires more Ruby gems...

# Working with Scons

Note: from the source distribution, move src/main.scons to the root of the
package.

`main.scons` sets up all the product flags and includes subdirectory scons
files. Because we use swtoolkit, our scons files contain a mix of swtoolkit
functions and scons functions.

Resources:

* [swtoolkit examples](https://code.google.com/p/swtoolkit/wiki/Examples)
* [swtoolkit glossary](https://code.google.com/p/swtoolkit/wiki/Glossary#COMPONENT_LIBRARY_PUBLISH)
* [scons manual](http://www.scons.org/doc/production/HTML/scons-man.html)
