/*
** This file is part of hbootdbg.
** Copyright (C) 2013 Cedric Halbronn <cedric.halbronn@sogeti.com>
** Copyright (C) 2013 Nicolas Hureau <nicolas.hureau@sogeti.com>
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** * Redistributions of source code must retain the above copyright notice, this
**   list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright notice, this
**   list of conditions and the following disclaimer in the documentation and/or
**   other materials provided with the distribution.
**
** * Neither the name of the {organization} nor the names of its
**   contributors may be used to endorse or promote products derived from
**   this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
** ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __HBOOTLIB_H__
# define __HBOOTLIB_H__

# include <stddef.h>

/*
** HBOOT functions
*/

extern __attribute__((long_call)) void __memcpy(void*, void*, int);
extern __attribute__((long_call)) void* __memset(void* s, int c, uint n);
extern __attribute__((long_call)) int __fb_cmd_oem(char*);
extern __attribute__((long_call)) int __setting_set_bootmode(uint);
extern __attribute__((long_call)) void __do_reboot();
extern __attribute__((long_call)) void __usb_send_string(char* str);
extern __attribute__((long_call)) void __usb_send(char* data, uint len);
extern __attribute__((long_call)) uint __usb_recv(char* data, uint len);
extern __attribute__((long_call)) unsigned int __strtol16(uchar* str);
extern __attribute__((long_call)) int __strlen(char* str);
extern __attribute__((long_call)) void __fastboot_reboot();
extern __attribute__((long_call)) void __turn_on_flashlight(int s);

/*
** Wrappers / replacements
*/

void* memmove(void*, const void*, size_t);
void* memcpy(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);

#endif // __HBOOTLIB_H__
