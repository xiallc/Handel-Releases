#
# Copyright (c) 2015 XIA LLC
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
#
# All logic relating to what flags enable/disable what products and vice-versa
# belongs in here to keep it out of the main build tools, since, by definition,
# this logic is just messy and isn't critical to understanding how the rest of
# the build works.
#

def check_platform_support(env, items):
    """
    Checks the compatibility of the items with the provided build environment.
    :param env: The SCons environment containing the information we need.
    :param items: A dictionary whose keys correspond to bits that have or can be
                  set in the provided environment, which need checked.
    :return: None
    """
    for item in [i for i in items.keys() if env.Bit(i)]:
        archs = items[item].get(env['PLATFORM'], [])
        if not archs:
            env.ClearBits(item)
        else:
            if env['TARGET_ARCH'] not in archs:
                env.ClearBits(item)


def update_environment(env, build_bits):
    """
    Disables devices, protocols, or libraries based on the provided environment.
    :param env: The SCons environment containing the information we need.
    :param build_bits: A dictionary containing the known devices, protocols,
                       libraries, and build options.
    :return: Nothing
    """
    check_platform_support(env, build_bits['libraries'])
    check_platform_support(env, build_bits['protocols'])

    for device in [d for d in build_bits['devices'].keys() if env.Bit(d)]:
        protocols = build_bits['devices'][device]['protocols']
        libs = build_bits['devices'][device].get('libraries', [])
        if (not [p for p in protocols if env.Bit(p)] or
                (libs and not [lib for lib in libs if env.Bit(lib)])):
            env.ClearBits(device)
