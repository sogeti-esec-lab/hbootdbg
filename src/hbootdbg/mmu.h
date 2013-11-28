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

#ifndef __MMU_H__
# define __MMU_H__

# include <stddef.h>

# define MMU_CONTROL_ENABLE             (1 << 0)
# define MMU_CONTROL_ALIGN_CHECK        (1 << 1)
# define MMU_CONTROL_L1_DATA_CACHE_ENABLE (1 << 2)
# define MMU_CONTROL_WRITE_BUFFER       (1 << 3)
# define MMU_CONTROL_SYSTEM_PROTECT     (1 << 8)
# define MMU_CONTROL_ROM_PROTECT        (1 << 9)
# define MMU_CONTROL_BRANCH_PREDICT_ENABLE (1 << 11)
# define MMU_CONTROL_L1_INSTRUCTION_CACHE_ENABLE (1 << 12)
# define MMU_CONTROL_HIGH_VECTORS       (1 << 13)
# define MMU_CONTROL_CACHE_RR_STRATEGY  (1 << 14)

# define MMU_CONTROL_EXTENDED_PAGE_TABLE (1 << 23)
# define MMU_CONTROL_EXCEPTION_ENDIAN   (1 << 25)

# define MMU_PAGE_SECTION_SHIFT 20
# define MMU_PAGE_SECTION_SIZE (1 << MMU_PAGE_SECTION_SHIFT)

# define MMU_PAGE_TYPE_UNMAPPED 0
# define MMU_PAGE_TYPE_COARSE 1
# define MMU_PAGE_TYPE_SECTION 2
# define MMU_PAGE_TYPE_FINE 3

# define ARM_SPR_MASK_FIQ               (1 << 6)
# define ARM_SPR_MASK_IRQ               (1 << 7)
# define ARM_SPR_MASK_INTS              (ARM_SPR_MASK_FIQ | ARM_SPR_MASK_IRQ)

typedef union
{
    unsigned int i;

    struct __packed
    {
        unsigned int isize_len : 2;
        unsigned int isize_m : 1;
        unsigned int isize_assoc : 3;
        unsigned int isize_size : 3;
        unsigned int isize_sbz : 3;

        unsigned int dsize_len : 2;
        unsigned int dsize_m : 1;
        unsigned int dsize_assoc : 3;
        unsigned int dsize_size : 3;
        unsigned int dsize_sbz : 3;

        unsigned int separate : 1;
        unsigned int ctype : 4;
        unsigned int sbz : 3;
    } bits;
} cache_type_register;

typedef union
{
    unsigned int i;

    struct __packed
    {
        unsigned int type : 2;
        unsigned int b : 1;
        unsigned int c : 1;
        unsigned int xn : 1; /* ARMv6 */
        unsigned int domain : 4;
        unsigned int imp : 1;
        unsigned int ap : 2;
        unsigned int tex : 3;
        unsigned int apx : 1; /* ARMv6 */
        unsigned int s : 1;
        unsigned int ng : 1;
        unsigned int super : 1;
        unsigned int sbz : 1;
        unsigned int base_address : 12;
    } bits;
} mmu_section_descriptor;

/*
** General MMU operations
*/

void mmu_enable(void);
void mmu_disable(void);

cache_type_register mmu_get_cache_type_register(void);
mmu_section_descriptor* mmu_get_translation_table(void);

/*
** Data cache operations
*/

uint mmu_get_dcache_line_size(void);
void mmu_invalidate_dcache_line(void* addr);
void mmu_invalidate_dcache_range(void* addr, size_t size);

/*
** Instruction cache operations
*/

uint mmu_get_icache_line_size(void);
void mmu_invalidate_icache_line(void* addr);
void mmu_invalidate_icache_range(void* addr, size_t size);

/*
** Both caches operations
*/

void mmu_invalidate_cache_line(void* addr);
void mmu_invalidate_cache_range(void* addr, size_t size);

/*
** Verify if memory is readable/writable
*/

int mmu_probe_read(void* addr, size_t length);
int mmu_probe_write(void* addr, size_t length);

#endif // __MMU_H__
