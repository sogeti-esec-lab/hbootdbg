/*
** This file is part of hbootdbg.
** Copyright (C) 2013 Cedric Halbronn <cedric.halbronn@sogeti.com>
** Copyright (C) 2013 Nicolas Hureau <nicolas.hureau@sogeti.com>
** All rights reserved.
**
** Code greatly inspired by qcombbdbg.
** Copyright (C) 2012 Guillaume Delugré <guillaume@security-labs.org>
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
#include "entrypoint.h"
#include "hboot.h"
#include "asm.h"

#define PRELOAD_OEM_HANDLER_ADDR 0x8D0E0000
#define SIZEOF_FUNCTION 0x1000

typedef int (*t_my_fb_cmd_oem_dbg_handler)(char*);

__attribute__((long_call))
t_my_fb_cmd_oem_dbg_handler my_fb_cmd_oem_dbg_handler =
    (t_my_fb_cmd_oem_dbg_handler) PRELOAD_OEM_HANDLER_ADDR;

/*
 * Important: I instructed GCC to preserve code order (-fno-toplevel-reorder).
 * Do not place anything before the first function in order to preserve this
 * as the entry point.
 */
void __attribute__((naked)) entrypoint_preloader()
{
    ARM_ASSEMBLY(
            "stmfd sp!, {r0-r12}"
            );

    replace_version("HACK");
    flush_instruction_cache();

    copy_preloader_functions();
    hook_fb_cmd_oem();

    flush_instruction_cache();

    ARM_ASSEMBLY(
            "ldmfd sp!, {r0-r12}"
            );

    //restore old context by jumping back after fastboot_getvar() call
    //we only return 0 to indicate it was successfully handled and do not care about saved registers but seems OK for now.
    //HTC Desire S - saga_0.98.0002
    /*ARM_ASSEMBLY(
      "mov r0, #0\n"
      "ldr pc, fastboot_getvar_ret\n"
      "fastboot_getvar_ret:\n"
      "	.word 0x8D003D78\n"
      );*/
    //HTC Desire Z - vision_0.85.0015 (htc unlock)
    ARM_ASSEMBLY(
            "mov r0, #0\n"
            "ldr pc, fastboot_getvar_ret\n"
            "fastboot_getvar_ret:\n"
            "	.word 0x8D003A8C\n"
            );

}

// arg1 = absolute address where to get the 4 bytes to use as fake
void replace_version(char* version)
{
    //__memcpy((void*)g_version_bootloader_addr, version, 5);
    __memcpy((void*)0x8D000004, version, 5);
}

/*
 * Copy the main code (preloader) to an area where we have space.
 */
void copy_preloader_functions()
{
    __memcpy((void*)PRELOAD_OEM_HANDLER_ADDR, my_fb_cmd_oem_dbg, SIZEOF_FUNCTION);
}

/*
 * Copy instructions from our replacing payload to the fb_cmd_oem original
 * function
 * We have 18 instructions below => size=18x4 bytes
 * 18x4=72 bytes available in this function
 * 8D0023B0-8D00236C+4=0x48=72
 */
void hook_fb_cmd_oem()
{
    __memcpy(__fb_cmd_oem, my_fb_cmd_oem, 18*4);
}


/*
 * Code replacement for fb_cmd_oem function that handles "fastboot oem"
 * requests. We choose to basically jump to our real handler in another area
 * to have more space for our function.
 *
 * We need to be standalone and not depend on other parts because we will be
 * copied in place of the original fb_cmd_oem function.
 */
__attribute__((naked))
int my_fb_cmd_oem(char* cmd)
{
    (void) cmd;

    ARM_ASSEMBLY(
            "stmfd sp!, {r4, lr}\n"
            "mov lr, pc\n"
            "ldr pc, my_fb_cmd_oem_dbg_handler_addr\n"
            "ldmfd sp!, {r4,pc}\n"
            "my_fb_cmd_oem_dbg_handler_addr:\n"
            "    .word 0x8D0E0000\n"
    );
    //return my_fb_cmd_oem_dbg_handler(cmd); //also works but depends on addresses in BSS that may be overwritten!
    return 0;
}
