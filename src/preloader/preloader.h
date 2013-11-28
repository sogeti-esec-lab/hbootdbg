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

#ifndef __PRELOADER_H__
#define __PRELOADER_H__

#include <stddef.h>

#define ERROR_SUCCESS 0
#define ERROR_CMD_NOT_FOUND -1
#define ERROR_INVALID_MEMORY_ACCESS -8

/*enum packet_type 
{
  PACKET_RESPONSE,
  PACKET_EVENT
};*/

enum cmd_type 
{
    /* Basic debugger commands */
    CMD_READ_MEM = 7,
    CMD_WRITE_MEM = 8,
};

/* Command structures */
//char hooked_cmd[3]; //"oem" missing because removed before being given to handler
typedef struct __attribute__((packed, aligned(4)))
{
    char cmd_type;
    union 
    {
        struct {
            void * base;
            size_t size;
        } read;

        struct {
            void * dest;
            size_t size;
            char data[1];
        } write;
    };
} request_packet;

typedef struct __attribute__((packed, aligned(4)))
{
    char type;
    char error_code;
    char data[1];

} response_packet;

int __attribute__((long_call)) my_fb_cmd_oem_dbg(char* cmd);
void cmd_default();
void cmd_read_memory(void* addr, size_t count);
void cmd_write_memory(void* dest, void* data, size_t size);

void __attribute__((naked)) flush_instruction_cache();

#endif // __PRELOADER_H__
