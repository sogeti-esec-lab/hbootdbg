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

#include "mmu.h"

/*
** Retrieves MMU cache type register.
*/
cache_type_register mmu_get_cache_type_register(void)
{
    cache_type_register cache_type;

    ASM(
        "mrc p15, 0, %0, c0, c0, 1\n"
        : "=r" (cache_type.i)
    );

    return cache_type;
}

/*
** Retrieves the length in bytes of a single data cache line.
*/
uint mmu_get_dcache_line_size(void)
{
    return (1 << (mmu_get_cache_type_register().bits.dsize_len + 3));
}

/*
** Retrieves the length in bytes of a single instruction cache line.
*/
uint mmu_get_icache_line_size(void)
{
    return (1 << (mmu_get_cache_type_register().bits.isize_len + 3));
}

/*
** Invalidate a single line in the data cache (commits cache to memory)
*/
void mmu_invalidate_dcache_line(void* addr)
{
    ASM(
        "mcr p15, 0, %0, c7, c10, 1\n"
        :: "r" (addr)
    );
}

/*
** Invalidate a memory range in the data cache.
*/
void mmu_invalidate_dcache_range(void* addr, size_t size)
{
    void* line;
    uint cache_line_size = mmu_get_dcache_line_size();

    for (line = (void*) ((int) addr & ~(cache_line_size - 1));
            line < (addr + size);
            line += cache_line_size)
    {
        mmu_invalidate_dcache_line(line);
    }
}

/*
** Invalidates a single line in the instruction cache.
*/
void mmu_invalidate_icache_line(void* addr)
{
    ASM(
        "mcr p15, 0, %0, c7, c5, 1\n"
        :: "r" (addr)
    );
}

/*
** Invalidates a memory range in the instruction cache.
*/
void mmu_invalidate_icache_range(void* addr, size_t size)
{
  void* line;
  unsigned int cache_line_size = mmu_get_icache_line_size();

  for (line = (void*) ((int) addr & ~(cache_line_size - 1));
          line < (addr + size);
          line += cache_line_size )
  {
      mmu_invalidate_icache_line(line);
  }
}

/*
** Invalidate a single line in both instruction and data caches.
*/
void mmu_invalidate_cache_line(void* addr)
{
    mmu_invalidate_icache_line(addr);
    mmu_invalidate_dcache_line(addr);
}

/*
** Invalidate a memory range in both instruction and data caches.
*/
void mmu_invalidate_cache_range(void* addr, size_t size)
{
    mmu_invalidate_icache_range(addr, size);
    mmu_invalidate_dcache_range(addr, size);
}


/*
** Enable the memory management unit.
*/
void mmu_enable(void)
{
    unsigned int tmp_reg;

    ASM(
        "mrc p15, 0, %0, c1, c0, 0\n"
        "orr %0, %0, #0x1\n"
        "mcr p15, 0, %0, c1, c0, 0\n"
        : "=r" (tmp_reg)
    );
}

/*
** Disable the memory management unit.
*/
void mmu_disable(void)
{
    unsigned int tmp_reg;

    ASM(
        "mrc p15, 0, %0, c1, c0, 0\n"
        "bic %0, %0, #0x1\n"
        "mcr p15, 0, %0, c1, c0, 0\n"
        : "=r" (tmp_reg)
    );
}

/*
**  Retrieves MMU control register.
*/
unsigned int mmu_get_control_register(void)
{
    unsigned int mmu_ctrl;

    ASM(
        "mrc p15, 0, %0, c1, c0, 0\n"
        : "=r" (mmu_ctrl)
    );

    return mmu_ctrl;
}

/*
** Retrieves page translation table base register (TTBR0).
*/
mmu_section_descriptor* mmu_get_translation_table(void)
{
    mmu_section_descriptor* page_table;

    ASM(
        "mrc p15, 0, %0, c2, c0, 0\n" /* assume we use TTBR0 here */
        "lsr %0, %0, #14\n"
        "lsl %0, %0, #14\n"
        : "=r" (page_table)
    );

    return page_table;
}

/*
** Checks if a memory area is readable.
** ARMv5 only, assumes APX is not implemented.
** Assumes section granularity is used.
*/
int mmu_probe_read(void* addr, size_t length)
{
    mmu_section_descriptor* ttbr;
    mmu_section_descriptor* section_desc_entry;
    uint mmu_ctrl;
    uint sections_nr, section_end;

    ttbr = mmu_get_translation_table();
    mmu_ctrl = mmu_get_control_register();

    /* Computes number of impacted sections */
    sections_nr = 0;
    section_end = ((uint)addr + MMU_PAGE_SECTION_SIZE) & ~(MMU_PAGE_SECTION_SIZE - 1);

    if (length > section_end - (uint)addr)
    {
        sections_nr++;
        length -= section_end - (uint)addr;
    }

    /* length / MMU_PAGE_SECTION_SIZE */
    sections_nr += (length >> MMU_PAGE_SECTION_SHIFT);

    /* length % MMU_PAGE_SECTION_SIZE */
    if (length - ((length >> MMU_PAGE_SECTION_SHIFT) << MMU_PAGE_SECTION_SHIFT))
        sections_nr++;

    /* Checks access right for each section */
    for (uint i = 0; i < sections_nr; i++)
    {
        section_desc_entry = &ttbr[((uint)addr >> MMU_PAGE_SECTION_SHIFT) + i];

        if (section_desc_entry->bits.type == MMU_PAGE_TYPE_UNMAPPED)
            return 0;

        if (!(mmu_ctrl & (MMU_CONTROL_SYSTEM_PROTECT | MMU_CONTROL_ROM_PROTECT)))
        {
            if (section_desc_entry->bits.ap == 0)
                return 0;
        }
    }

    return 1;
}

/*
** Checks if a memory area is writable.
** ARMv5 only, assumes APX is not implemented.
** Assumes section granularity is used.
*/
int mmu_probe_write(void* addr, size_t length)
{
    mmu_section_descriptor* ttbr;
    mmu_section_descriptor* section_desc_entry;
    uint sections_nr, section_end;

    ttbr = mmu_get_translation_table();

    /* Computes number of impacted sections */
    sections_nr = 0;
    section_end = ((uint)addr + MMU_PAGE_SECTION_SIZE) & ~(MMU_PAGE_SECTION_SIZE - 1);

    if (length > section_end - (uint)addr)
    {
        sections_nr++;
        length -= section_end - (uint)addr;
    }

    /* length / MMU_PAGE_SECTION_SIZE */
    sections_nr += (length >> MMU_PAGE_SECTION_SHIFT);

    /* length % MMU_PAGE_SECTION_SIZE */
    if (length - ((length >> MMU_PAGE_SECTION_SHIFT) << MMU_PAGE_SECTION_SHIFT))
        sections_nr++;

    /* Checks access right for each section */
    for (uint i = 0; i < sections_nr; i++)
    {
        section_desc_entry = &ttbr[((uint)addr >> MMU_PAGE_SECTION_SHIFT) + i];

        if (section_desc_entry->bits.type == MMU_PAGE_TYPE_UNMAPPED)
            return 0;

        if (section_desc_entry->bits.ap == 0)
            return 0;
    }

    return 1;
}
