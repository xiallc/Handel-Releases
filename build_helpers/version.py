# -*- coding: iso-8859-1 -*-
#
# Copyright (c) 2009 XIA LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms,
# with or without modification, are permitted provided
# that the following conditions are met:
#
#   * Redistributions of source code must retain the above
#     copyright notice, this list of conditions and the
#     following disclaimer.
#   * Redistributions in binary form must reproduce the
#     above copyright notice, this list of conditions and the
#     following disclaimer in the documentation and/or other
#     materials provided with the distribution.
#   * Neither the name of X-ray Instrumentation Associates
#     nor the names of its contributors may be used to endorse
#     or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# Helper routines to generate the version header for Handel.
#

import yaml
import datetime
import subprocess
import os.path


def generate(target, source, env):
    """
    Generates a version header file suitable for use with Handel. env should
    contain a path to the version metadata (in YAML format) at the key
    'version_metadata'.
    """
    version = None
    with open(str(source[0]), 'r') as f:
        version = yaml.safe_load(f)

    assert (version != None)

    revstring = None
    if os.path.isfile(str(source[1])):
        with open(str(source[1]), 'r') as f:
            revstring = yaml.safe_load(f)['versionstring']

    assert (revstring != None)

    save_file = str(target[0])

    if 'xia_version.h' in save_file:
        write_version_h(save_file, version, revstring)
    elif 'handel.rc' in save_file:
        build_options = ','.join(env['build_options'])
        write_handel_rc(save_file, version, revstring, build_options)
    else:
        print("Unsupprted target %s " % save_file)
        assert (False)


def get_changeset():
    return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('utf-8').strip()


def write_handel_rc(file, version, revstring, build_options):
    version_cs = "%s,%s,%s" % (version['major'], version['minor'], version['revision'])
    if revstring != "":
        version_ps = "%s.%s.%s - %s" % (version['major'], version['minor'],
                                        version['revision'], revstring)
    else:
        version_ps = "%s.%s.%s" % (version['major'], version['minor'], version['revision'])

    with open(file, 'w') as f:

        f.write("#define HANDEL_FILEVERSION %s\n" % version_cs)
        f.write("#define HANDEL_PRODUCTVERSION_STRING \"%s\"\n" % version_ps)
        f.write("#define HANDEL_BUILD_OPTIONS \"Build options: %s\"\n" % build_options)

        f.write("""#include <windows.h>

// DLL version information.
VS_VERSION_INFO    VERSIONINFO
FILEVERSION        HANDEL_FILEVERSION
PRODUCTVERSION     HANDEL_FILEVERSION
FILEFLAGSMASK      VS_FFI_FILEFLAGSMASK
#ifdef _DEBUG
  FILEFLAGS        VS_FF_DEBUG | VS_FF_PRERELEASE
#else
  FILEFLAGS        0
#endif
FILEOS             VOS_NT_WINDOWS32
FILETYPE           VFT_DLL
FILESUBTYPE        VFT2_UNKNOWN
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904b0"
    BEGIN
      VALUE "Comments",         HANDEL_BUILD_OPTIONS
      VALUE "CompanyName",      "XIA LLC"
      VALUE "FileDescription",  "Handel API library for XIA DXP hardware"
      VALUE "InternalName",     "Handel"
      VALUE "LegalCopyright",   "� XIA LLC"
      VALUE "OriginalFilename", "handel.dll"
      VALUE "ProductName",      "Handel Library"
      VALUE "ProductVersion",   HANDEL_PRODUCTVERSION_STRING
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x0409, 1200
  END
END
""")


def write_version_h(file, version, revstring):
    with open(file, 'w') as f:
        f.write("/* xia_version.h -- DO NOT MODIFY\n")
        f.write(" *\n")
        f.write(" * This file automatically generated by \n")
        f.write(" * build_helpers.version on %s\n" % datetime.datetime.now())
        f.write(" */\n")
        f.write("\n")
        f.write("#ifndef __XIA_VERSION_H__\n")
        f.write("#define __XIA_VERSION_H__\n")
        f.write("\n")
        f.write("#define XERXES_MAJOR_VERSION %s\n" % version['major'])
        f.write("#define XERXES_MINOR_VERSION %s\n" % version['minor'])
        f.write("#define XERXES_RELEASE_VERSION %s\n" % version['revision'])
        f.write("\n")
        f.write("#define HANDEL_MAJOR_VERSION %s\n" % version['major'])
        f.write("#define HANDEL_MINOR_VERSION %s\n" % version['minor'])
        f.write("#define HANDEL_RELEASE_VERSION %s\n" % version['revision'])
        f.write("\n")
        f.write("#define VERSION_STRING \"%s\"\n" % revstring)
        f.write("\n")
        f.write("#endif /* __XIA_VERSION_H__ */\n")
        f.write("\n")


def update_version_changeset(version_yml, rev_yml, version_tag):
    """
    Update the revision hash in the rev_yml file, add an optional
    version_tag in front of the revision hash, only trigger a
    rebuild if revision or version_tag was updated in versionstring
    """
    if os.path.isfile(version_yml):
        with open(version_yml, 'r') as f:
            version = yaml.safe_load(f)
    full_version_string = version_tag + get_changeset()
    set_version_string(rev_yml, full_version_string)


def set_version_string(rev_yml, newstring):
    revstring = {'versionstring': ""}
    if os.path.isfile(rev_yml):
        with open(rev_yml, 'r') as f:
            revstring = yaml.safe_load(f)

    versionstring = revstring.get('versionstring', "")

    if versionstring != newstring:
        revstring['versionstring'] = newstring
        with open(rev_yml, 'w') as f:
            f.write(yaml.dump(revstring))
