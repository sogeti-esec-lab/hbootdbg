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

#include "hbootlib.h"

#ifdef REUSE_EXTERNAL_STRING_FUNCS

void* memmove(void* dest, const void* src, size_t n)
{
    //return __memmove(dest, src, n);
    return __memcpy(dest, src, n);
}

void* memcpy(void* dest, const void* src, size_t n)
{
    return __memcpy(dest, src, n);
}

void* memset(void* s, int c, size_t n)
{
    return __memset(s, c, n);
}

size_t strlen(const char* s)
{
    return __strlen(s);
}

#else

void* memmove(void* dest, const void* src, size_t n)
{
    size_t i;

    if (dest < src)
    {
        for (i = 0; i < n; ++i)
            ((char*) dest)[i] = ((char*) src)[i];
    }
    else if (dest > src)
    {
        for (i = n; i > 0; --i)
            ((char*) dest)[i - 1] = ((char*) src)[i - 1];
    }

    return dest;
}

void* memcpy(void* dest, const void* src, size_t n)
{
    return memmove(dest, src, n);
}

void* memset(void* s, int c, size_t n)
{
    while (n--)
        ((char*) s)[n] = c;

    return s;
}

size_t strlen(const char *s)
{
        const char *c;

        for (c = s; *c; ++c)
                ;

        return (c - s);
}

#endif

