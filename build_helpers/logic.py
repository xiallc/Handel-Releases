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
#


def update_environment(env):
    """
    Configures the proper EXCLUDE flags for passed in environment based
    on the protcol/device bits that are set.
    """
    win = env['PLATFORM'] == 'win32' # The target_platform tool may not have set proper bits yet.

    if win and env.Bit('x64'):
        env.ClearBits('usb')
        env.ClearBits('epp')
        env.ClearBits('serial')

    if not win:
        env.ClearBits('plx', 'xw')

    if not env.Bit('epp') and not env.Bit('usb') and not env.Bit('usb2'):
        env.ClearBits('saturn')

    if not env.Bit('serial') and not env.Bit('usb2'):
        env.ClearBits('udxp')
        env.ClearBits('udxps')

    if not env.Bit('usb2'):
        env.ClearBits('mercury')

    if not env.Bit('plx'):
        env.ClearBits('xmap')
        env.ClearBits('stj')

    # We don't know if the user specified things from the device/protocol
    # point-of-view or some combination therein. Some of these will overlap
    # with the protocol logic. It's okay.

    if not env.Bit('saturn'):
        env.ClearBits('epp')
        env.ClearBits('usb')

    if not env.Bit('saturn') and \
            not env.Bit('mercury') and not env.Bit('udxp') \
            and not env.Bit('udxps'):
        env.ClearBits('usb2')

    if not env.Bit('udxp') and not env.Bit('udxps'):
        env.ClearBits('serial')
        env.ClearBits('xw')

    if not env.Bit('xmap') and not env.Bit('stj'):
        env.ClearBits('plx')

    print "build options: ",

    for option_item in ['usb','usb2','epp','plx','serial']:
        if env.Bit(option_item):
            print option_item,

    for option_item in ['stj','xmap','saturn','mercury','udxp','udxps']:
        if env.Bit(option_item):
            print option_item,
    print
