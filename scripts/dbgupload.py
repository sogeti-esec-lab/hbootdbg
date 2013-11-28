#! /usr/bin/env python3

# This file is part of hbootdbg.
# Copyright (c) 2013, Cedric Halbronn <cedric.halbronn@sogeti.com>
# Copyright (c) 2013, Nicolas Hureau <nicolas.hureau@sogeti.com>
# All right reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
# 
# * Neither the name of the {organization} nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import binascii
import os
import struct
import sys
import time
from hboot import HbootClient

PRELOADER_PATH = '../src'
DEBUGGER_PATH = '../src'

TTY='/dev/ttyUSB0'

##
# Preloader commands
##

READ_MEM = 7
WRITE_MEM = 8

##
# Supported firmware revisions.
##

# cmd_hook: address of function to hook
# preload_addr: where the preloader is uploaded (device specific)
DEVICE_OFFSETS = {
    'vision_0.85.0005'  : {
        'fb_oem_hook'   : 0x0, # FIXME
        'hb_keytest_hook' : 0x0, # FIXME
        'preloader'     : 0x0, # FIXME
        'payload'       : 0x0, # FIXME
    },
    'vision_0.85.0015'  : {
        'fb_oem_hook'   : 0x8D0020C8,
        'hb_keytest_hook' : 0x8D010DF0,
        'preloader'     : 0x058D3000,
        'payload'       : 0x8D0E2000, # Where the debugger is uploaded
    },
    'saga_0.98.0002'    : {
        'fb_oem_hook'   : 0x8D00236C,
        'hb_keytest_hook' : 0x0, # FIXME
        'preloader'     : 0x069D3000,
        'payload'       : 0x0, # FIXME
    },
}

def dbg_read_memory(fbc, addr, size):
  cmd = struct.pack('=BII', READ_MEM, addr, size)
  data = fbc.preloader(cmd)
  return data

def dbg_write_memory(fbc, dest, data):
  (res1, res2) = fbc.download_data(data)
  cmd = struct.pack('=BII', WRITE_MEM, dest, len(data))
  data = fbc.preloader(cmd)
  return data

def step(msg, step_function, *args, out=sys.stderr, continue_on_fail=False):
    out.write('[*] %s' % msg)
    out.flush()

    (success, error_msg) = step_function(*args);
    if success:
        out.write('\r[+] %s: %s\n' % (msg, error_msg))
    else:
        out.write('\r[!] %s: %s\n' % (msg, error_msg))

    out.flush()

    if not success and not continue_on_fail:
        sys.exit(1)

##
# Get HBOOT version
##

def get_hboot_version(fbc):
    global offsets

    (res1, version0) = fbc.getvar('version-bootloader')
    (res2, product) = fbc.getvar('product')

    version='%s_%s' % (product.decode(), version0.decode())

    if res1 != b'OKAY' or res2 != b'OKAY':
        return (False, 'Unknown device revision %s, aborting' % version)

    offsets = DEVICE_OFFSETS[version]

    return (True, version)

##
# Copy preloader stub
##

def copy_preloader(fbc):
    (res1,res2) = fbc.download('%s/preloader.bin' % PRELOADER_PATH)
    return (True, 'OK')

##
# Trigger revolutionary exploit
##

def trigger_exploit(fbc):
    (res, version_main) = fbc.getvar('version-main')
    return (True, 'OK')

##
# Check if exploit successful
##

def check_exploit_result(fbc):
    (res, version0) = fbc.getvar('version-bootloader')
    return (version0 == b'HACK', version0.decode())

##
# We can now use the preloader function to inject the real payload faster.
##

def copy_hbootdbg(fbc):
    data = b''
    with open('%s/hbootdbg.bin' % DEBUGGER_PATH, 'br') as f:
        data = dbg_write_memory(fbc, offsets['payload'], f.read())
    return (True, 'OK')

##
# Patch fastboot oem and hboot keytest to point at final payload
##

#                        patch:
# 10 40 2D E9                 STMFD   SP!, {R4,LR}
# 0F E0 A0 E1                 MOV     LR, PC
# 00 F0 1F E5                 LDR     PC, =0x8D0E2000
# 10 80 BD E8                 LDMFD   SP!, {R4,PC}
# 00 00 0E 8D                 DCD 0x8D0E2000

def patch_cmds(fbc):
    global offsets

    patch =  b'\x10\x40\x2D\xE9'
    patch += b'\x0F\xE0\xA0\xE1'
    patch += b'\x00\xF0\x1F\xE5'
    patch += b'\x10\x80\xBD\xE8'
    patch += struct.pack('I', offsets['payload'])

    data = dbg_write_memory(fbc, offsets['hb_keytest_hook'], patch)
    data = dbg_write_memory(fbc, offsets['fb_oem_hook'], patch)

    return (True, 'OK')

if __name__ == '__main__':
    fbc = None

    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()

    def connect_to_device(args):
        global fbc
        not_connected = True
        while not_connected:
            try:
                fbc = HbootClient(debug=args.debug)
                not_connected = False
            except Exception as e:
                time.sleep(1)

        return (True, 'OK')

    step('Wait for device', connect_to_device, args)
    step('Get HBOOT version', get_hboot_version, fbc)
    step('Copy preloader', copy_preloader, fbc)
    step('Trigger Revolutionary exploit', trigger_exploit, fbc)
    step('Get HBOOT version', check_exploit_result, fbc)
    step('Copy hbootdbg', copy_hbootdbg, fbc)
    step('Patch fastboot oem and hboot keytest commands', patch_cmds, fbc)

    fbc.close()
