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

#include "dbg.h"

#include "base64.h"
#include "cpu.h"
#include "hbootlib.h"
#include "int.h"
#include "mmu.h"
#include "reloc.h"
#include "string.h"

static dbg_status status;

// Forward declarations
breakpoint* get_breakpoint(void* addr);

void dbg_init(void)
{
    memset(&status, 0, sizeof (status));
}

/*
** Event handlers
*/

static
void breakpoint_handler(context* ctx)
{
    char buf[1024];
    char decoded_buf[1024];
    command* cmd;
    uint read_len;

    breakpoint* bp = get_breakpoint((void*) (ctx->pc));
    status.continue_address =
        bp == NULL ? ctx->pc + 4 : (u32) bp->original_instruction;

    while (1)
    {
        do {
            read_len = __usb_recv(buf, sizeof (buf)); // non blocking
        } while (read_len == 0);

        if (buf[0] == 'o')
            base64_decode(buf + 4, decoded_buf); // Command is received as
                                                 // "oem CMD", "buf + 4" strips
                                                 // "oem "
        else
            base64_decode(buf + 8, decoded_buf); // Command is received as
                                                 // "keytest CMD", "buf + 8"
                                                 // strips "keytest "

        cmd = (command*) decoded_buf;
        cmd_dispatcher(cmd, ctx);

        // Continue execution, and also if the debugger is detaching
        if (cmd->type == CMD_BREAKPOINT_CONTINUE || cmd->type == CMD_DETACH)
            break;
    }
}

void dbg_event_handler(event_type event, context* ctx)
{
    switch (event)
    {
    case EVENT_BREAKPOINT:
        breakpoint_handler(ctx);
        break;

    default:
        __fastboot_reboot();
        break;
    }
}

/*
** Breakpoint
*/
breakpoint* alloc_breakpoint(void)
{
    if (status.bp_size >= DBG_NBR_POINTS)
        return NULL;

    status.bp_size += 1;

    return &(status.bp[status.bp_size - 1]);
}

/*
** Freeing a breakpoint potentially invalidates all current breakpoint
** pointers
*/
void free_breakpoint(breakpoint* bp)
{
    for (uint i = 0; i < status.bp_size; ++i)
    {
        if (status.bp[i].address == bp->address)
        {
            status.bp[i] = status.bp[status.bp_size - 1];
            status.bp_size -= 1;
            return;
        }
    }
}

breakpoint* get_breakpoint(void* addr)
{
    for (uint i = 0; i < status.bp_size; ++i)
    {
        if (status.bp[i].address == addr)
            return &(status.bp[i]);
    }

    return NULL;
}

error_code insert_breakpoint(void* addr, breakpoint_type type)
{
    if (get_breakpoint(addr))
        return ERROR_BREAKPOINT_ALREADY_EXISTS;

    breakpoint* bp = alloc_breakpoint();
    if (bp == NULL)
        return ERROR_NO_MEMORY_AVAILABLE;

    bp->type    = type;
    bp->address = addr;
    bp->enabled = 1;
    bp->original_instruction[0] = *(u32*) addr;

    // This solution is temporary as it just won't work with instructions
    // that manipulate PC
    bp->original_instruction[1] = cpu_get_branch(
            (u32) &(bp->original_instruction[1]),
            (u32) &((u32*) addr)[1]
    );

    // Invalidate caches
    mmu_invalidate_cache_line(&bp->original_instruction[0]);
    mmu_invalidate_cache_line(&bp->original_instruction[1]);

    *(u32*) addr = ARM_BKPT;
    mmu_invalidate_cache_line(addr);

    return ERROR_SUCCESS;
}

error_code remove_breakpoint(void* addr, breakpoint_type type)
{
    (void) type;

    breakpoint* bp = get_breakpoint(addr);
    if (bp == NULL)
        return ERROR_NO_BREAKPOINT;

    *(u32*) addr = bp->original_instruction[0];
    mmu_invalidate_cache_line(addr);

    free_breakpoint(bp);

    return ERROR_SUCCESS;
}

/*
** Commands
*/

void cmd_dispatcher(command* cmd, context* ctx)
{
    switch (cmd->type)
    {
    case CMD_ATTACH:
        cmd_attach(cmd, ctx);
        break;

    case CMD_DETACH:
        cmd_detach(cmd, ctx);
        break;

    case CMD_READ:
        cmd_read(cmd, ctx);
        break;

    case CMD_WRITE:
        cmd_write(cmd, ctx);
        break;

    case CMD_INSERT_BREAKPOINT:
        cmd_insert_breakpoint(cmd, ctx);
        break;

    case CMD_REMOVE_BREAKPOINT:
        cmd_remove_breakpoint(cmd, ctx);
        break;

    case CMD_GET_REGISTERS:
        cmd_get_registers(cmd, ctx);
        break;

    case CMD_BREAKPOINT_CONTINUE:
        cmd_breakpoint_continue(cmd, ctx);
        break;

    case CMD_CALL:
        cmd_call(cmd, ctx);
        break;

    case CMD_BREAKPOINT:
        cmd_breakpoint(cmd, ctx);
        break;

    case CMD_FLASHLIGHT:
        cmd_flashlight(cmd, ctx);
        break;

    case CMD_FASTBOOT_REBOOT:
        cmd_fastboot_reboot(cmd, ctx);
        break;

    default:
        cmd_error(cmd, ERROR_UNKNOWN_CMD);
        break;
    }
}

inline void cmd_success(command* cmd)
{
    cmd_error(cmd, ERROR_SUCCESS);
}

void cmd_error(command* cmd, error_code error)
{
    cmd->error = error;
    __usb_send((char*) cmd, 2);
}

void cmd_attach(command* cmd, context* ctx)
{
    (void) ctx;

    dbg_init();
    int_install_exception_handlers();
    cmd_success(cmd);
}

void cmd_detach(command* cmd, context* ctx)
{
    (void) ctx;

    cmd_success(cmd);
}

void cmd_read(command* cmd, context* ctx)
{
    (void) ctx;

    u8* addr = cmd->read.addr;
    u32 count = cmd->read.size;
    u32 block_size = 1024;
    u32 len;

    if (!mmu_probe_read(addr, count))
    {
        cmd_error(cmd, ERROR_UNMAPPED_MEMORY);
        return;
    }

    cmd_success(cmd);

    while (count > 0)
    {
        len = count < block_size ? count : block_size;
        __usb_send((char*) addr, len);
        count -= len;
        addr += len;
    }
}

void cmd_write(command* cmd, context* ctx)
{
    (void) ctx;

    void* addr = cmd->write.addr;
    u32 size = cmd->write.size;
    u8* data = cmd->write.data;

    if (!mmu_probe_read(addr, size))
    {
        cmd_error(cmd, ERROR_UNMAPPED_MEMORY);
        return;
    }

    memcpy(addr, data, size);
    mmu_invalidate_cache_range(addr, size);

    cmd_success(cmd);
}

void cmd_insert_breakpoint(command* cmd, context* ctx)
{
    (void) ctx;

    error_code err =
        insert_breakpoint(cmd->breakpoint.addr, cmd->breakpoint.type);

    cmd_error(cmd, err);
}

void cmd_remove_breakpoint(command* cmd, context* ctx)
{
    (void) ctx;

    error_code err =
        remove_breakpoint(cmd->breakpoint.addr, cmd->breakpoint.type);

    cmd_error(cmd, err);
}

void cmd_get_registers(command* cmd, context* ctx)
{
    if (ctx == NULL)
        cmd_error(cmd, ERROR_NO_BREAKPOINT);
    else
    {
        cmd_success(cmd);
        __usb_send((char*) ctx, sizeof (*ctx));
    }
}

void cmd_breakpoint_continue(command* cmd, context* ctx)
{
    if (ctx == NULL)
        cmd_error(cmd, ERROR_NO_BREAKPOINT);
    else
    {
        ctx->pc = status.continue_address;
        cmd_success(cmd);
    }
}

void cmd_breakpoint(command* cmd, context* ctx)
{
    (void) ctx;

    cmd_success(cmd);
    ASM("bkpt\n");
}

void cmd_flashlight(command* cmd, context* ctx)
{
    (void) ctx;

    cmd_success(cmd);
    __turn_on_flashlight(cmd->flashlight.time);
}

void cmd_call(command* cmd, context* ctx)
{
    (void) ctx;

    void* addr = cmd->call.addr;
    u32* args = cmd->call.args;

    ASM(
        "push {r0-r4}\n"
        "ldr r4, %[function]\n"
        "ldr r0, %[arg0]\n"
        "ldr r1, %[arg1]\n"
        "ldr r2, %[arg2]\n"
        "ldr r3, %[arg3]\n"
        "blx r4\n"
        "pop {r0-r4}\n"
        :
        : [function] "m" (addr),
          [arg0] "m" (args[0]),
          [arg1] "m" (args[1]),
          [arg2] "m" (args[2]),
          [arg3] "m" (args[3])
    );
}

void cmd_fastboot_reboot(command* cmd, context* ctx)
{
    (void) cmd;
    (void) ctx;

    __fastboot_reboot();
}
