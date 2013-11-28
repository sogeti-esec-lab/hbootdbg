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

#ifndef __CPU_H__
# define __CPU_H__

# define ARM_BKPT               0xe1200070

# define ARM_MODE_USER          0b10000
# define ARM_MODE_FIQ           0b10001
# define ARM_MODE_IRQ           0b10010
# define ARM_MODE_SVC           0b10011
# define ARM_MODE_ABORT         0b10111
# define ARM_MODE_UNDEF         0b11011
# define ARM_MODE_SYS           0b11111
# define ARM_MODE_MONITOR       0b10110

# define ARM_SPR_MASK_MODE      0b11111
# define ARM_SPR_THUMB          (1 << 5)
# define ARM_SPR_MASK_FIQ       (1 << 6)
# define ARM_SPR_MASK_IRQ       (1 << 7)
# define ARM_SPR_MASK_INTS      (ARM_SPR_MASK_FIQ | ARM_SPR_MASK_IRQ)
# define ARM_SPR_COND_FLAGS     (0b11111 << 27)

/*
** Structs
*/

typedef struct __packed
{
    u32 cpsr;
    union
    {
        struct
        {
            u32 r0;
            u32 r1;
            u32 r2;
            u32 r3;
            u32 r4;
            u32 r5;
            u32 r6;
            u32 r7;
            u32 r8;
            u32 r9;
            u32 r10;
            u32 r11;
            u32 r12;
            u32 sp;
            u32 lr;
            u32 pc;
        };
        u32 r[16];
    };
} context;

/*
** Functions
*/

void cpu_set_mode_stack(uint mode, void* addr);

/*
** Helpers
*/

u32 cpu_get_cpsr(void);
void cpu_put_cpsr(u32 cpsr);

u32 cpu_get_branch(u32 from, u32 to);

#endif // __CPU_H__
