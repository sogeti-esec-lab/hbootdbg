#! /usr/bin/env python3

# This file is part of hbootdbg.
# Copyright (c) 2013, Cedric Halbronn <cedric.halbronn@sogeti.com>
# Copyright (c) 2013, Nicolas Hureau <nicolas.hureau@sogeti.com>
# Copyright (c) 2013, Pierre-Marie de Rodat <pmderodat@kawie.fr>
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
import hbootdbg
import re
import serial
import socket
import struct
import sys
import time

class PatternDispatcher:
    ''' Call handlers according to regular expression matching '''

    def __init__(self):
        self.handlers = []
        self.owner = None

    def register(self, regexp, handler):
        self.handlers.append((
            re.compile(regexp),
            handler
        ))

    def registered(self, regexp):
        def decorator(func):
            self.register(regexp, func)
            return func
        return decorator

    def __get__(self, instance, owner):
        self.owner = instance
        return self

    def dispatch(self, command, *data_list):
        for cmd_regexp, handler in reversed(self.handlers):
            match = cmd_regexp.match(command)
            if match:
                return handler(self.owner, match, *data_list)
        return None

class TCPServer:
    ''' Class to listen to GDB using TCP as a medium '''

    def __init__(self, addr='127.0.0.1', port=1234):
        self._addr = addr
        self._port = port
        self._s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    def start(self):
        self._s.bind((self._addr, self._port))
        self._s.listen(1)
        self._conn, self._client_addr = self._s.accept()

    def stop(self):
        self._conn.close()

    def recv(self, size):
        return self._conn.recv(size)

    def send(self, bytes):
        self._conn.send(bytes)

class SerialServer:
    ''' Class to listen to GDB using serial as a medium '''

    def __init__(self, tty='/dev/ttyUSB0', speed=9600, timeout=1):
        self._s = serial.Serial(tty, speed, timeout, port=None)

    def start(self):
        self._s.open()

    def stop(self):
        self._s.close()

    def recv(self, size):
        data = self._s.read(1)
        n = self._s.inWaiting()
        if size > 1 and n > 0:
            data += self._s.read(min(size, n))
        return data

    def send(self, bytes):
        self._s.write(bytes)

class ARMRegisters:
    ''' ARM registers storage, packing and unpacking facility '''

    def __init__(self):
        self._cpsr = 0
        self._gpr = [0 for _ in range(16)]
        self._fpr = [0 for _ in range(8)]

    def __getitem__(self, i):
        if i >= 0 and i <= 15:
            return self._gpr[i]
        elif i >= 16  and i <= 23:
            return self._fpr[i]
        elif i == 25:
            return self._cpsr
        else:
            raise ValueError

    def cpsr(self):
        return self._cpsr

    def unpack(self, data):
        self._cpsr = struct.unpack_from('>I', data, offset=0)[0]
        for i in range(len(self._gpr)):
            self._gpr[i] = struct.unpack_from("<I", data, offset=4+(i*4))[0]

        # XXX: to bypass r11 being dereferenced by GDB (considered as fp?)
        self._gpr[11] = 0x8d05e8c0

    def pack_gdb(self):
        data = []
        for r in self._gpr:
            data.append('{:08x}'.format(
                struct.unpack('>I', struct.pack('<I', r))[0]))
        for r in self._fpr:
            data.append('{:08x}'.format(
                struct.unpack('>I', struct.pack('<I', r))[0]))
        data.append('{:08x}'.format(0))
        data.append('{:08x}'.format(
            struct.unpack('>I', struct.pack('<I', self._cpsr))[0]))
        return ''.join(data).encode()

class GDBServer:
    ''' GDB server handling requests form the client debugger and routing
    them to the host '''

    DISPATCHER = PatternDispatcher()

    def __init__(
            self,
            server,
            tty='/dev/ttyUSB0',
            ferr=sys.stderr,
            first_run=False,
            fastboot_mode=False,
            debug=False):
        self._server = server
        self._dbg = hbootdbg.HbootDbg(tty,
                fastboot_mode=fastboot_mode,
                debug=debug)
        self._ferr = ferr
        self._enable_debug = debug
        self._first_run = first_run
        self._r = ARMRegisters()

    def debug(self, *args, **kwargs):
        if self._enable_debug:
            kwargs['file'] = self._ferr
            print('GDBSERVER:', *args, **kwargs)

    def read(self, size):
        result = self._server.recv(size)
        return result

    def send(self, *data_list):
        self.debug('Sending:', self.pack(data_list))
        self._server.send(self.pack(data_list))

    def send_raw(self, data):
        self.debug('Sending:', data)
        self._server.send(data)

    def expect(self, bytes):
        buffer = self._server.recv(len(bytes))
        assert buffer == bytes, '{} != {}'.format(buffer, bytes)

    def unpack(self, data, checksum):
        checksum = checksum.lower()
        if (
            len(checksum) != 2 or
            not (ord(b'0') <= checksum[0] <= ord(b'f')) or
            not (ord(b'0') <= checksum[1] <= ord(b'f')) or
            sum(c for c in data) % 256 != int(checksum, 16)
        ):
            return None
        else:
            result = []
            current_word = []
            escaped = False
            for i, c in enumerate(data):
                # TODO: handle length-encoding
                if escaped:
                    current_word.append(c ^ 0x20)
                    escaped = False
                elif c == b'}':
                    escaped = True
                elif c in b',:;':
                    result.append(bytes(current_word))
                    current_word = []
                else:
                    current_word.append(c)
            result.append(bytes(current_word))
            return result

    def pack(self, data_list):
        packed_data_list = []
        for data in data_list:
            packed_data = []
            packed_data_list.append(packed_data)
            for c in data:
                if c in b'$#}':
                    packed_data.append(ord(b'}'))
                    c ^= 0x20
                packed_data.append(c)
        packed_data = b';'.join(
            bytes(packed_data)
            for packed_data in packed_data_list
        )
        checksum = '{:02x}'.format(sum(c for c in packed_data) % 256)
        return b'$' + bytes(packed_data) + b'#' + checksum.encode('ascii')

    def run(self):
        if self._first_run:
            self._dbg.attach()
            self._dbg.breakpoint()
        self._server.start()
        while True:
            packet_type = self.read(1)
            if len(packet_type) == 0:
                self.debug('Connection closed')
                break

            if packet_type == b'$':
                data = b''
                while True:
                    byte = self.read(1)
                    if byte != b'#':
                        data += byte
                    else:
                        break
                checksum = self.read(2)
                data = self.unpack(data, checksum)
                if data is None:
                    self.debug('Received corrupted packet:', data)
                    self.send_raw(b'-')
                else:
                    self.debug('Received packet:', data)
                    self.send_raw(b'+')
                    if len(data) < 1:
                        continue
                    command, data_list = data[0], data[1:]
                    if not self.DISPATCHER.dispatch(command, *data_list):
                        self.debug('  -> Unhandled packet')
                        self.send(b'')

            elif packet_type == b'+':
                pass
            elif packet_type == b'-':
                self.debug('Host received a corrupted packet')
            elif packet_type == b'\x03':
                self.debug('Host sent interruption')
            else:
                self.debug('Received invalid packet type: {}'.format(packet_type))
        self._server.stop()

    @DISPATCHER.registered(b'^qSupported$')
    def handle_qsupported(self, cmd_match, *data_list):
        features = [
            b'PacketSize=1024'
        ]
        self.send(*features)
        return True

    @DISPATCHER.registered(b'^qAttached$')
    def handle_qattached(self, cmd_match, *data_list):
        self.send(b'1')
        return True

    @DISPATCHER.registered(b'^qC$')
    def handle_q_current_thread(self, cmd_match, *data_list):
        self.send(b'QC0')
        return True

    @DISPATCHER.registered(b'^H(m|M|g|G|c|C)(-?\d+)$')
    def handle_set_thread(self, cmd_match, *data_list):
        match = cmd_match.group(0)
        operation = cmd_match.group(1)
        thread_id = cmd_match.group(2)
        if operation == b'c':
            self.send(b'OK')
        else:
            self.send('')
        return True

    @DISPATCHER.registered(br'^\?$')
    def handle_get_stop_reason(self, cmd_match, *data_list):
        self.send(b'S05')
        return True

    @DISPATCHER.registered(b'^g$')
    def handle_get_registers(self, cmd_match, *data_list):
        self._r.unpack(self._dbg.get_registers().data)
        self.send(self._r.pack_gdb())
        return True

    @DISPATCHER.registered(b'^p([0-9A-Fa-f]+)$')
    def handle_get_register(self, cmd_match, *data_list):
        reg_nbr = int(cmd_match.group(1), 16)
        self.send('{:08x}'.format(self._r[reg_nbr]).encode())
        return True

    @DISPATCHER.registered(b'^c$')
    def handle_continue(self, cmd_match, *data_list):
        self._dbg.breakpoint_continue()
        while self._dbg.get_registers().error == hbootdbg.ERROR_NO_BREAKPOINT:
            time.sleep(0.1)
        self.send(b'S05')
        return True

    @DISPATCHER.registered(b'^s$')
    def handle_step(self, cmd_match, *data_list):
        break_pc = self._r[15] + 4
        print('   => INSERT BP: {:08x}'.format(break_pc))
        self._dbg.insert_breakpoint(break_pc)
        self._dbg.breakpoint_continue()
        while self._dbg.get_registers().error == hbootdbg.ERROR_NO_BREAKPOINT:
            time.sleep(0.05)
        print('   => DELETE BP: {:08x}'.format(break_pc))
        self._dbg.remove_breakpoint(break_pc)
        self.send(b'S05')
        return True

    @DISPATCHER.registered(b'^m([0-9A-Fa-f]+)$')
    def handle_read_memory(self, cmd_match, *data_list):
        address = int(cmd_match.group(1), 16)
        size = int(data_list[0], 16)
        data = self._dbg.read(address, size).data
        self.send(binascii.hexlify(data))
        return True

    @DISPATCHER.registered(b'^M([0-9A-Fa-f]+)$')
    def handle_write_memory(self, cmd_match, *data_list):
        address = int(cmd_match.group(1), 16)
        size = int(data_list[0], 16)
        data = int(data_list[1], 16)
        res = self._dbg.write(address, size, data).data
        self.send(binascii.hexlify(res))
        return True

    @DISPATCHER.registered(b'^Z0$')
    def handle_insert_breakpoint(self, cmd_match, *data_list):
        address = int(data_list[0], 16)
        type = int(data_list[1])
        self._dbg.insert_breakpoint(address)
        self.send(b'OK')
        return True

    @DISPATCHER.registered(b'^z0$')
    def handle_remove_breakpoint(self, cmd_match, *data_list):
        address = int(data_list[0], 16)
        type = int(data_list[1])
        self._dbg.remove_breakpoint(address)
        self.send(b'OK')
        return True

    @DISPATCHER.registered(b'^D$')
    def handle_detach(self, cmd_match, *data_list):
        #self._dbg.breakpoint_continue()
        self._dbg.detach()
        return True

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', '--listen', type=str, default='127.0.0.1')
    parser.add_argument('-p', '--port', type=int, default=1234)
    parser.add_argument('-r', '--first-run', action='store_true')
    parser.add_argument('-f', '--fastboot-mode', action='store_true')
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()

    server = TCPServer(args.listen, args.port)
    proxy = GDBServer(server,
                first_run=args.first_run,
                fastboot_mode=args.fastboot_mode,
                debug=args.debug)

    try:
        proxy.run()
    except KeyboardInterrupt as e:
        pass
