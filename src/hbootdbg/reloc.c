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

#include "reloc.h"

#include "darm/darm.h"

int darm_armv7_update(darm_t* d)
{
    d->w = 0;

    d->w |= (d->cond << 28);
    switch (d->instr)
    {
    case I_B:
        d->w |= (0b1010 << 24);
        d->w |= (d->imm & 0xffffff);
        break;

    case I_BL:
        d->w |= (0b1011 << 24);
        d->w |= (d->imm & 0xffffff);
        break;

    default:
        return -1;
    };

    return 0;
}

int relocate_instruction(u32* instr,
                         u32 src_addr,
                         u32 dst_addr)
{
    darm_t d;

    if (darm_armv7_disasm(&d, *instr) != 0)
        return -1;

    switch (d.instr)
    {
    case I_B:
    case I_BL:
        d.imm += (src_addr - dst_addr);
        break;

    default:
        return -1;
    };

    if (darm_armv7_update(&d) != 0)
        return -1;

    *instr = d.w;

    return 0;
}
