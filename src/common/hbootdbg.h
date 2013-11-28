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

#ifndef __HBOOTDBG_H__
# define __HBOOTDBG_H__

# include <stdint.h>

# define ASM(code, args...) do { \
        __asm__ __volatile__( \
            code, ## args \
        ); \
    } while (0)

# define __packed __attribute__((packed))
# define __aligned4 __attribute__((aligned(4)))
# define __naked __attribute__((naked))

# ifndef offsetof
#  define offsetof __builtin_offsetof
# endif

typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;
typedef unsigned long long ulonglong;
typedef int8_t          s8;
typedef int16_t         s16;
typedef int32_t         s32;
typedef int64_t         s64;
typedef uint8_t         u8;
typedef uint16_t        u16;
typedef uint32_t        u32;
typedef uint64_t        u64;

#endif // __HBOOTDBG_H__
