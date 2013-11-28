/*
Copyright (c) 2013, Jurriaan Bremer
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the darm developer(s) nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "darm.h"
#include "darm-internal.h"

#define APPEND(out, ptr) \
    do { \
        const char *p = ptr; \
        if(p != NULL) while (*p != 0) *out++ = *p++; \
    } while (0);

void darm_init(darm_t *d)
{
    // initialize the entire darm state in order to make sure that no members
    // contain undefined data
    memset(d, 0, sizeof(darm_t));
    d->instr = I_INVLD;
    d->instr_type = T_INVLD;
    d->shift_type = S_INVLD;
    d->S = d->E = d->U = d->H = d->P = d->I = B_INVLD;
    d->R = d->T = d->W = d->M = d->N = d->B = B_INVLD;
    d->Rd = d->Rn = d->Rm = d->Ra = d->Rt = R_INVLD;
    d->Rt2 = d->RdHi = d->RdLo = d->Rs = R_INVLD;
    d->option = O_INVLD;
    // TODO set opc and coproc? to what value?
    d->CRn = d->CRm = d->CRd = R_INVLD;
    d->firstcond = C_INVLD, d->mask = 0;
}

int darm_reglist(uint16_t reglist, char *out)
{
    char *base = out;

    if(reglist == 0) return -1;

    *out++ = '{';

    while (reglist != 0) {
        // count trailing zero's
        int32_t reg, start = __builtin_ctz(reglist);

        // most registers have length two
        *(uint16_t *) out = *(uint16_t *) darm_registers[start];
        out[2] = darm_registers[start][2];
        out += 2 + (out[2] != 0);

        for (reg = start; reg == __builtin_ctz(reglist); reg++) {
            // unset this bit
            reglist &= ~(1 << reg);
        }

        // if reg is not start + 1, then this means that a series of
        // consecutive registers have been identified
        if(reg != start + 1) {
            // if reg is start + 2, then this means that two consecutive
            // registers have been found, but we prefer the notation
            // {r0,r1} over {r0-r1} in that case
            *out++ = reg == start + 2 ? ',' : '-';
            *(uint16_t *) out = *(uint16_t *) darm_registers[reg-1];
            out[2] = darm_registers[reg-1][2];
            out += 2 + (out[2] != 0);
        }
        *out++ = ',';
    }

    out[-1] = '}';
    *out = 0;
    return out - base;
}
