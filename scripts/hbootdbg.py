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

##
# Commands
##

COMMAND = {
    'undefined'         : 0,
    'attach'            : 1,
    'detach'            : 2,
    'read'              : 3,
    'write'             : 4,
    'insert_breakpoint' : 5,
    'remove_breakpoint' : 6,
    'breakpoint_continue' : 7,
    'get_registers'     : 8,

    # Debug
    'call'              : 50,
    'fastboot_reboot'   : 51,
    'breakpoint'        : 55,
    'flashlight'        : 80,
}

##
# Errors
##

ERROR_SUCCESS                   = 0
ERROR_UNKNOWN_CMD               = 1
ERROR_MALFORMED_CMD             = 2
ERROR_INVALID_MEMORY_ACCESS     = 3
ERROR_BREAKPOINT_ALREADY_EXISTS = 4
ERROR_NO_BREAKPOINT             = 5
ERROR_NO_MEMORY_AVAILABLE       = 6
ERROR_UNMAPPED_MEMORY           = 7

ERROR = {
    ERROR_SUCCESS               : 'SUCCESS',
    ERROR_UNKNOWN_CMD           : 'UNKNOWN_CMD',
    ERROR_MALFORMED_CMD         : 'MALFORMED_CMD',
    ERROR_INVALID_MEMORY_ACCESS : 'INVALID_MEMORY_ACCESS',
    ERROR_BREAKPOINT_ALREADY_EXISTS : 'BREAKPOINT_ALREADY_EXISTS',
    ERROR_NO_BREAKPOINT         : 'NO_BREAKPOINT',
    ERROR_NO_MEMORY_AVAILABLE   : 'NO_MEMORY_AVAILABLE',
    ERROR_UNMAPPED_MEMORY       : 'UNMAPPED_MEMORY',
}

##
# Breakpoints
##

BREAKPOINT_NORMAL               = 0
BREAKPOINT_TRACE                = 1

BREAKPOINT = {
    BREAKPOINT_NORMAL           : 'BREAKPOINT_NORMAL',
    BREAKPOINT_TRACE            : 'BREAKPOINT_TRACE',
}

##
# Commands packing/unpacking
##

class Command:
    def __init__(self,
            type = COMMAND['undefined'],
            error = 0,
            address = 0,
            size = 0,
            breakpoint_type = 0,
            data = None,
            args = (0, 0, 0, 0),
            time_ = 0):
        self.type = type
        self.error = error
        self.address = address
        self.size = size
        self.breakpoint_type = breakpoint_type
        self.data = data
        self.args = args
        self.time_ = time_

    def pack(self):
        packed =  struct.pack('B', self.type)
        packed += struct.pack('B', self.error)

        if self.type == COMMAND['read']:
            packed += struct.pack('I', self.address)
            packed += struct.pack('I', self.size)

        elif self.type == COMMAND['write']:
            packed += struct.pack('I', self.address)
            packed += struct.pack('I', self.size)
            # XXX: Can only write 4 bytes at a time
            packed += struct.pack('>I', self.data)

        elif self.type == COMMAND['insert_breakpoint']:
            packed += struct.pack('I', self.address)
            packed += struct.pack('I', self.breakpoint_type)

        elif self.type == COMMAND['remove_breakpoint']:
            packed += struct.pack('I', self.address)
            packed += struct.pack('I', self.breakpoint_type)

        elif self.type == COMMAND['call']:
            packed += struct.pack('I', self.address)
            packed += struct.pack('I', self.args[0])
            packed += struct.pack('I', self.args[1])
            packed += struct.pack('I', self.args[2])
            packed += struct.pack('I', self.args[3])

        elif self.type == COMMAND['flashlight']:
            packed += struct.pack('I', self.time_)

        return packed

    def unpack(self, packed):
        self.type = struct.unpack_from('B', packed, offset=0)[0]
        self.error = struct.unpack_from('B', packed, offset=1)[0]

        if self.type == COMMAND['read']:
            self.data = packed[2:]

        if self.type == COMMAND['write']:
            self.data = packed[2:]

        if self.type == COMMAND['breakpoint']:
            self.data = packed[2:]

        if self.type == COMMAND['get_registers']:
            self.data = packed[2:]

        return self

##
# HbootDbg interface
##

class HbootDbg:

    def __init__(self, tty='/dev/ttyUSB0',
                       fastboot_mode=True,
                       debug=False):
        not_connected = True
        sys.stderr.write("Waiting for device...")
        sys.stderr.flush()
        while not_connected:
            try:
                self._client = HbootClient(tty,
                    blocking_io=False,
                    fastboot_mode=fastboot_mode,
                    debug=debug
                )
                not_connected = False
            except Exception as e:
                time.sleep(1)
        sys.stderr.write("\r                     \r")

    def attach(self):
        cmd = Command(COMMAND['attach'])
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def detach(self):
        cmd = Command(COMMAND['detach'])
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def read(self, address, size):
        cmd = Command(COMMAND['read'],
                address=address,
                size=size)
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def write(self, address, size, data):
        cmd = Command(COMMAND['write'],
                address=address,
                size=size,
                data=data)
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def insert_breakpoint(self, address, type=BREAKPOINT_NORMAL):
        cmd = Command(COMMAND['insert_breakpoint'],
                address=address, breakpoint_type=type)
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def remove_breakpoint(self, address, type=BREAKPOINT_NORMAL):
        cmd = Command(COMMAND['remove_breakpoint'],
                address=address, breakpoint_type=type)
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def breakpoint_continue(self):
        cmd = Command(COMMAND['breakpoint_continue'])
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def get_registers(self):
        cmd = Command(COMMAND['get_registers'])
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def call(self, address, args = (0, 0, 0, 0)):
        cmd = Command(COMMAND['call'],
                address=address,
                args=args)
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def breakpoint(self):
        cmd = Command(COMMAND['breakpoint'])
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def flashlight(self, time_):
        cmd = Command(COMMAND['flashlight'], time_)
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def fastboot_reboot(self):
        cmd = Command(COMMAND['fastboot_reboot'])
        return Command().unpack(self._client.hbootdbg(cmd.pack()))

    def raw(self, data):
        return self._client.hbootdbg(data.to_bytes(4, sys.byteorder))

    def console(self):
        while True:
            cmd = input('hbootdbg> ').split()

            if cmd[0] not in CONSOLE_COMMAND:
                print('Unknown command %s' % cmd[0])
                continue

            if len(cmd) != CONSOLE_COMMAND[cmd[0]].nb_args + 1:
                print('Usage: %s' % CONSOLE_COMMAND[cmd[0]].usage)
                continue

            args = tuple([int(arg, 16) for arg in cmd[1:]])
            response = CONSOLE_COMMAND[cmd[0]].func(self, *args)

            if response.error != ERROR_SUCCESS:
                print(ERROR[response.error])

            if isinstance(response, Command) and response.data != None:
                print(binascii.hexlify(response.data))
            elif not isinstance(response, Command):
                print(binascii.hexlify(response))

##
# Console
##

class ConsoleCommandHelper:
    def __init__(self, nb_args, usage, func):
        self.nb_args = nb_args
        self.usage = usage
        self.func = func

CONSOLE_COMMAND = {
    'undefined'         : ConsoleCommandHelper(0, 'undefined', None),
    'attach'            : ConsoleCommandHelper(0, 'attach', HbootDbg.attach),
    'detach'            : ConsoleCommandHelper(0, 'detach', HbootDbg.detach),
    'read'              : ConsoleCommandHelper(2, 'read addr size', HbootDbg.read),
    'write'             : ConsoleCommandHelper(3, 'write addr size data', HbootDbg.write),
    'insert_breakpoint' : ConsoleCommandHelper(1, 'insert_breakpoint addr', HbootDbg.insert_breakpoint),
    'remove_breakpoint' : ConsoleCommandHelper(1, 'remove_breakpoint addr', HbootDbg.remove_breakpoint),
    'continue'          : ConsoleCommandHelper(0, 'breakpoint_continue', HbootDbg.breakpoint_continue),
    'get_registers'     : ConsoleCommandHelper(0, 'get_registers', HbootDbg.get_registers),

    # Debug
    'call'              : ConsoleCommandHelper(4, 'call arg1 arg2 arg3 arg4', HbootDbg.call),
    'break'             : ConsoleCommandHelper(0, 'breakpoint', HbootDbg.breakpoint),
    'light'             : ConsoleCommandHelper(1, 'flashlight', HbootDbg.flashlight),
    'reboot'            : ConsoleCommandHelper(0, 'fastboot_reboot', HbootDbg.fastboot_reboot),
    'raw'               : ConsoleCommandHelper(1, 'raw', HbootDbg.raw),
}

def get_branch_instr(target, start_addr):
    return struct.pack('=I', 0xea000000 | \
        (((target & 0x00ffffff) - (start_addr & 0x00ffffff) - 8) >> 2))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--fastboot-mode', action='store_true')
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()

    dbg = HbootDbg(fastboot_mode=args.fastboot_mode, debug=args.debug)

    try:
        dbg.console()
    except KeyboardInterrupt as e:
        pass
