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

#ifndef __DBG_H__
# define __DBG_H__

# include "cpu.h"
# include "int.h"
# include <stddef.h>

#define DBG_NBR_POINTS  64

typedef enum
{
    ERROR_SUCCESS               = 0,
    ERROR_UNKNOWN_CMD           = 1,
    ERROR_MALFORMED_CMD         = 2,
    ERROR_INVALID_MEMORY_ACCESS = 3,
    ERROR_BREAKPOINT_ALREADY_EXISTS = 4,
    ERROR_NO_BREAKPOINT         = 5,
    ERROR_NO_MEMORY_AVAILABLE   = 6,
    ERROR_UNMAPPED_MEMORY       = 7,
} error_code;

typedef enum
{
    BREAKPOINT_NORMAL           = 0,
    BREAKPOINT_TRACE            = 1,
} breakpoint_type;

typedef struct
{
    breakpoint_type type;
    void* address;
    u32 original_instruction[2];
    char enabled;
} breakpoint;

typedef struct
{
    char initialized;

    breakpoint bp[DBG_NBR_POINTS];
    uint bp_size;

    u32 continue_address;
} dbg_status;

void dbg_init(void);

void dbg_break_handler(event_type, context*);

/*
** Commands understood by the debugger
*/

typedef enum
{
    CMD_UNDEFINED       = 0,
    CMD_ATTACH          = 1,
    CMD_DETACH          = 2,
    CMD_READ            = 3,
    CMD_WRITE           = 4,
    CMD_INSERT_BREAKPOINT = 5,
    CMD_REMOVE_BREAKPOINT = 6,
    CMD_BREAKPOINT_CONTINUE = 7,
    CMD_GET_REGISTERS   = 8,

    CMD_CALL            = 50,
    CMD_FASTBOOT_REBOOT = 51,
    CMD_BREAKPOINT      = 55,
    CMD_FLASHLIGHT      = 80,
} cmd_type;

/*
** Command structure
*/

typedef struct __attribute__((packed, aligned(4)))
{
    cmd_type type : 8;
    error_code error : 8;

    union
    {
        struct __packed
        {
            void* addr;
            u32 size;
        } read;

        struct __packed
        {
            void* addr;
            u32 size;
            u8 data[0];
        } write;

        struct __packed
        {
            void* addr;
            breakpoint_type type : 8;
        } breakpoint;

        struct __packed
        {
            void* addr;
            u32 args[4];
        } call;

        struct __packed
        {
            context ctx[0];
        } registers;

        struct __packed
        {
            u32 time;
        } flashlight;
    };
} command;

/*
** Functions
*/

void cmd_dispatcher(command*, context*);

void cmd_success(command*);
void cmd_error(command*, error_code);

void cmd_attach(command*, context*);
void cmd_detach(command*, context*);

void cmd_read(command*, context*);
void cmd_write(command*, context*);
void cmd_insert_breakpoint(command*, context*);
void cmd_remove_breakpoint(command*, context*);
void cmd_get_registers(command*, context*);
void cmd_breakpoint_continue(command*, context*);

void cmd_call(command*, context*);
void cmd_breakpoint(command*, context*);
void cmd_flashlight(command*, context*);
void cmd_fastboot_reboot(command*, context*);

#endif // __DBG_H__
