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

#include "preloader.h"
#include "hboot.h"
#include "asm.h"

/*
 * Important: I instructed GCC to preserve code order (-fno-toplevel-reorder).
 * The code of this file is copied somewhere else, so everything we need should
 * be here.
 */

int __attribute__((long_call)) my_fb_cmd_oem_dbg(char* cmd)
{
    request_packet* packet = (request_packet*) cmd;

    switch (packet->cmd_type)
    {
        case CMD_READ_MEM:
            cmd_read_memory(packet->read.base, packet->read.size);
            break;

        case CMD_WRITE_MEM:
            cmd_write_memory(
                packet->write.dest,
                (void*) 0x058D3000, /* hardcoded for vision_0.85.0015 */
                //(void*) 0x069D3000, /* hardcoded for saga_0.98.0002 */
                packet->write.size
            );
            break;

        default:
            cmd_default();
            break;
    }

    return 0;
}

void cmd_default()
{
    response_packet response;
    __memset(&response, 0, sizeof(response));
    response.error_code = ERROR_CMD_NOT_FOUND;
    __usb_send_to_device_1((char*) &response, sizeof (response));
}

void cmd_read_memory(void* addr, size_t count)
{
    unsigned int bsize = 1024;

    char* ptr = (char*) addr;
    unsigned int len;

    while (count > 0)
    {
        len = count < bsize ? count : bsize;
        __usb_send_to_device_1(ptr, len);
        count -= len;
        ptr += len;
    }
}

void cmd_write_memory(void* dest, void* data, size_t size)
{
    response_packet response;
    __memset(&response, 0, sizeof (response));

    __memcpy(dest, data, size);
    flush_instruction_cache(); /* in case we overwrote instructions */

    response.type = CMD_WRITE_MEM;
    response.error_code = ERROR_SUCCESS;
    __usb_send_to_device_1((char*)&response, sizeof(response));
}

/*
 * Flush the entire instruction cache
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0201d/ch03s02s03.html
 */
__attribute__((naked))
void flush_instruction_cache()
{
    ARM_ASSEMBLY(
        "mov r0, #0\n"
        "mcr p15, 0, r0, c7, c5, 0\n"
        "bx lr\n"
    );
}
