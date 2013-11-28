# hbootdbg

Debugger for HTC phones bootloader (HBOOT).

## Disclaimer

Currently only supports HTC Desire Z with HBOOT 0.85.0015.

## License

hbootdbg is released under the **BSD 3-Clause License**.

## Acknowledgement

* Jurriaan Bremer (@skier_t) for DARM disassembler (http://darm.re/)
* Base64 decoding provided by http://libb64.sourceforge.net

## Quick use guide

Enter fastboot mode in HBOOT.

```bash
~/hbootdbg $ cd src
~/hbootdbg/src $ make
[...]
~/hbootdbg $ cd ../scripts
~/hbootdbg/scripts $ sudo modprobe -r usbserial && sudo modprobe usbserial vendor=0x0BB4 product=0x0fff
~/hbootdbg/scripts $ ./dbgupload.py
[...]
~/hbootdbg/scripts $ ./hbootdbg.py -f
hbootdbg> attach
hbootdbg> break
b''
hbootdbg> get_registers
b'd301002002000000000000000000020000000000d82b098dbc2b098d01000000d82b098d2cce078d0d0a0d0a18ce078d0b0b0b0b10000000a4b81f8db494058dd4260e8d'
hbootdbg> read 0 4  
b'120000ea'
hbootdbg> continue
hbootdbg> detach
hbootdbg> ^C
~/hbootdbg/scripts $ ./gdbproxy.py -f -r & &> /dev/null
~/hbootdbg/scripts $ arm-none-eabi-gdb
[...]
(gdb) target remote :1234
Remote debugging using :1234
0x8d0e26d4 in ?? ()
(gdb) i r
r0             0x2	2
r1             0x0	0
r2             0x20000	131072
r3             0x0	0
r4             0x8d092bd8	2366188504
r5             0x8d092bbc	2366188476
r6             0x1	1
r7             0x8d092bd8	2366188504
r8             0x8d07ce2c	2366098988
r9             0xa0d0a0d	168626701
r10            0x8d07ce18	2366098968
r11            0x8d05e8c0	2365974720
r12            0x10	16
sp             0x8d1fb8a4	0x8d1fb8a4
lr             0x8d0594b4	2365953204
pc             0x8d0e26d4	0x8d0e26d4
cpsr           0x200001d3	536871379
```
