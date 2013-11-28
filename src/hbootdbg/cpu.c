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

#include "cpu.h"

__naked
void cpu_set_mode_stack(uint mode, void* addr)
{
    ASM(
        "stmfd sp!, {r3-r4, lr}\n"

        // Save the current CPSR
        "mrs r3, cpsr\n"

        // Move to the wanted CPU mode
        "bic r4, r3, #0x1F\n"
        "orr r4, %[mode]\n"
        "msr cpsr, r4\n"

        // Set the stack
        "mov sp, %[addr]\n"

        // Move back to the original mode
        "msr cpsr, r3\n"

        // Return
        "ldmfd sp!, {r3-r4, pc}\n"
        ::
        [mode] "r" (mode),
        [addr] "r" (addr)
    );
}

u32 cpu_get_cpsr(void)
{
    u32 tmp_reg;

    ASM(
        "mrs %0, cpsr\n"
        : "=r" (tmp_reg)
    );

    return tmp_reg;
}

void cpu_put_cpsr(u32 cpsr)
{
    ASM(
        "msr cpsr, %0\n"
        : "=r" (cpsr)
    );
}

u32 cpu_get_branch(u32 from, u32 to)
{
    return 0xea000000 | (((to - from - 8) >> 2) & 0x00ffffff);
}
