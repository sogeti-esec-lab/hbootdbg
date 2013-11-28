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

import binascii
import sys
import time
from serial import Serial

class SerialClient:
    def __init__(self, tty='/dev/ttyUSB0', blocking_io=True):
        self._s = Serial(tty, 9600, timeout=0.1)
        self._blocking_io = blocking_io

    def close(self):
        self._s.close()

    def read(self, size):
        if self._blocking_io:
            data = self._s.read(size)
        else:
            data = self._s.read(1)
            n = self._s.inWaiting()
            if size > 1 and n > 0:
                data += self._s.read(min(size, n))
        return data

    def write(self, data):
        self._s.write(data)

class HbootClient(SerialClient):
    def __init__(self, tty='/dev/ttyUSB0',
                       blocking_io=True,
                       fastboot_mode=True,
                       debug=False):
        super().__init__(tty, blocking_io)
        self._fastboot_mode = fastboot_mode
        self._debug = debug

    def raw(self, data):
        self.write(data)
        return self.read(1024)

    def getvar(self, var):
        var = var.encode()
        self.write(b'getvar:' + var)
        data = self.read(1000)
        return (data[:4], data[4:])

    def hbootdbg(self, cmd):
        if self._debug:
            print(b'plain: ' + cmd)
        cmd = binascii.b2a_base64(cmd)[:-1]
        if self._fastboot_mode:
            self.write(b'oem ' + cmd)
        else:
            self.write(b'keytest ' + cmd + b'\n')
        if self._debug:
            print(b'send: ' + cmd)
        data = b''
        while True:
            tmp = self.read(1024)
            if not tmp:
                break
            data += tmp
        if self._debug:
            print(b'recv: ' + data)
        if not self._fastboot_mode:
            # In hboot mode, command is echoed by the phone and it also adds the
            # hboot prompt at the end of the response. But we have to be careful
            # when entering a breakpoint, these additions are not there anymore,
            # we receive the raw responses
            data = data.split(b'\r\n')
            data = b'\r\n'.join(
                    data[1 if data[0].startswith(b'keytest') else None:
                    -1 if data[-1].endswith(b'hboot>') else None])
        if self._debug:
            print(b'stripped: ' + data)
        return data

    def preloader(self, cmd):
        if self._fastboot_mode:
            self.write(b'oem ' + cmd)
        else:
            self.write(b'keytest ' + cmd)
        if self._debug:
            print(cmd)
        data = b''
        while True:
            tmp = self.read(1024)
            if not tmp:
                break
            data += tmp
        if self._debug:
            print(data)
        return data

    def download(self,filename):
        data = open(filename, 'rb').read()
        return self.download_data(data)

    def download_data(self, data):
        len_str = '%08x' % len(data)
        self.write(b'download:' + len_str.encode())
        self.write(data)
        data = self.read(1000)
        return (data[:4], data[4:])

    def debug(self):
        print('DEBUG: ' + self.read(1000))

if __name__ == '__main__':
    s = SerialClient()
    while True:
        cmd = input('> ')
        s.write(cmd.encode() + b'\n')
        answer = s.read(1024).decode()
        print('{}'.format(answer))
