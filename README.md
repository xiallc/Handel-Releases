Handel is built with SCons+swtoolkit. The latest version of Scons
and swtoolkit are updated to be compatile with python3

1. Install the latest Python3 Windows installer from [python.org] or
   using Chocolately. The x86-64 version is fine on a 64-bit system.
2. Install required libraries
	pip install -r requirements.txt

Tests in t_api run with Ruby 2.3+. You will need both x86 and x64
versions of Ruby in order to test both builds of Handel. For each
version, `gem install ffi`.


Building with scons
===================

Handel has many flags that map to defines in the compilation environment. A
little experimentation may be needed to get the set that supports only the
products you need.

The incldued bulid script can be used to invoke SCons build environment.

Here's an example for microDXP USB2:

   build --udxp --no-udxps --no-xw --no-serial --no-xmap --no-stj --no-mercury --no-saturn --verbose

Another example to build the examples with default option:

   build --samples

To search for other flags, search "SetBitFromOption" in main.scons.



Resources:

* [python.org](https://www.python.org/downloads/)
* [swtoolkit examples](https://code.google.com/p/swtoolkit/wiki/Examples)
* [swtoolkit glossary](https://code.google.com/p/swtoolkit/wiki/Glossary#COMPONENT_LIBRARY_PUBLISH)
* [SCons manual](http://www.scons.org/doc/production/HTML/scons-man.html)
