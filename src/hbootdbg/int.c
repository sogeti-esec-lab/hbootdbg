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

#include "int.h"

#include "cpu.h"
#include "mmu.h"

interrupt_vector_table * ivt = 0x00000000;

#define INSTALL_EXCEPTION_HANDLER(vector, handler) do { \
        ivt->vector = cpu_get_branch( \
                offsetof(interrupt_vector_table, vector), (u32) handler & 0xffffff); \
        mmu_invalidate_icache_line( \
                (void*) offsetof(interrupt_vector_table, vector)); \
        mmu_invalidate_dcache_line( \
                (void*) offsetof(interrupt_vector_table, vector)); \
    } while (0)

void int_install_exception_handlers(void)
{
    // Prefetch abort handler
    INSTALL_EXCEPTION_HANDLER(prefetch_abort, &prefetch_abort_handler);
    cpu_set_mode_stack(ARM_MODE_ABORT, (void*) 0x8d0e0000); // XXX: use linker script
    // Undefined instruction handler
    //INSTALL_EXCEPTION_HANDLER(undefined_instruction, &prefetch_abort_handler);
    //cpu_set_mode_stack(ARM_MODE_UNDEF, (void*) 0x8d0e0000); // XXX: use linker script
    // Data abort handler
    //INSTALL_EXCEPTION_HANDLER(data_abort, &prefetch_abort_handler);
}

/*
** CALLING STACK:
** original r0-r12 registers
** original_lr
** pc = fault_address
** original cpsr
*/
__naked
void save_context(void)
{
    ASM(
        "stmfd sp!, {r0-r1}\n"          // save r0 on abort stack
        "sub r0, lr, #4\n"              // lr_abort = prefetch_abort_address + 4
        "mrs lr, spsr\n"
        "msr cpsr_c, lr\n"              // move to calling mode, disabled
                                        // interrupts
        "mov r1, sp\n"                  // save sp
        "stmfd sp!, {r0}\n"             // store pc when breaking (aka return address)
        "stmfd sp!, {lr}\n"             // store link register
        "stmfd sp!, {r1}\n"             // store stack pointer
        "stmfd sp!, {r2-r12}\n"         // store r2-r12
        "msr cpsr_c, %[abort_mode]\n"   // 11010111b : move to abort mode,
                                        // disabled interrupts
        "ldmfd sp!, {r0-r1}\n"          // restore original r0
        "mrs r2, spsr\n"
        "msr cpsr_c, lr\n"              // move to calling mode, disabled
                                        // interrupts
        "stmfd sp!, {r0-r1}\n"          // store r0-r1
        "stmfd sp!, {r2}\n"             // store spsr, return frame complete
        "msr cpsr_c, %[abort_mode]\n"   // 11010111b : move to abort mode,
                                        // disabled interrupts
        "ldmfd sp!, {lr}\n"             // Pop return address
        "subs pc, lr, #0\n"             // Return and move to calling mode
                                        // (return address was put on the stack
                                        // by the caller)
        ::
        [abort_mode] "i" (ARM_SPR_MASK_INTS | ARM_MODE_ABORT),
        [mask_ints] "i" (ARM_SPR_MASK_INTS)
    );
}

__naked
void restore_context(void)
{
    ASM(
        "ldmfd sp!, {r12}\n"
        "msr cpsr, r12\n"               // restore cpsr
        "ldmfd sp!, {r0-r12}\n"         // return to original context (r0-r12)
        "ldmfd sp!, {lr}\n"             // pop sp into nothing
        "ldmfd sp!, {lr, pc}\n"         // return to original context (lr, pc)
    );
}

__naked
void prefetch_abort_handler(void)
{
    ASM(
        "stmfd sp!, {pc}\n"             // Return address on the stack
        "b save_context\n"

        "mov r0, %[event]\n"            // r0 = EVENT_BREAKPOINT
        "mov r1, sp\n"                  // r1 = saved context

        "blx dbg_event_handler\n"       // call dbg_event_handler(
                                        // EVENT_BREAKPOINT, bkpt_context)
        "blx restore_context\n"

        ::
        [event] "i" (EVENT_BREAKPOINT)
    );
}
