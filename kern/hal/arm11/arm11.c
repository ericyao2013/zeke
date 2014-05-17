/**
 *******************************************************************************
 * @file    arm11.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM11
  * @{
  */

#define KERNEL_INTERNAL
#include <stddef.h>
#include <autoconf.h>
#include <kerror.h>
#include <errno.h>
#include "arm11.h"

volatile uint32_t flag_kernel_tick = 0;

void init_stack_frame(struct _ds_pthread_create * thread_def,
        sw_stack_frame_t * sframe, int priv)
{
    /* Note that scheduler has to have the same mapping. */
    uint32_t stack_start = ((uint32_t)(thread_def->def->stackAddr)
            + thread_def->def->stackSize
            - sizeof(errno_t));

    sframe->r0  = (uint32_t)(thread_def->argument);
    sframe->r1  = 0;
    sframe->r2  = 0;
    sframe->r3  = 0;
    sframe->r12 = 0;
    sframe->sp  = stack_start;
    sframe->pc  = ((uint32_t)(thread_def->start)) + 4;
    sframe->lr  = (uint32_t)(thread_def->del_thread);
    sframe->psr = priv ? SYSTEM_PSR : USER_PSR;
}

uint32_t syscall(uint32_t type, void * p)
{
    uint32_t scratch;

    __asm__ volatile (
        /* Lets expect that parameters are already in r0 & r1 */
        "SVC    #0\n\t"
        "MOV    %[res], r0\n\t"
        : [res]"=r" (scratch)
        : [typ]"r" (type), [arg]"r" (p)
        : "r2", "r3", "r4");

    return scratch;
}

istate_t get_interrupt_state(void)
{
    istate_t state;

    __asm__ volatile (
        "MRS %[reg], cpsr\n\t"
        "AND %[reg], %[reg], %[mask]"
        : [reg] "=r" (state)
        : [mask] "I" (PSR_INT_MASK));

    return state;
}

void set_interrupt_state(istate_t state)
{
    __asm__ volatile (
        "MRS r1, cpsr\n\t"
        "BIC r1, r1, %[mask]\n\t"
        "ORR r1, r1, %[ostate]\n\t"
        "MSR cpsr, r1"
        :
        : [mask] "I" (PSR_INT_MASK), [ostate] "r" (state)
        : "r1");
}

int test_and_set(int * lock)
{
    int err = 2; /* Initial value of error meaning already locked */

    istate_t s_entry = get_interrupt_state();
    disable_interrupt();

#if configMP == 0
    err = *lock != 0 ? 2 : 0;
    *lock = 1;
#else
    __asm__ volatile (
        "MOV        r1, #1\n\t"             /* locked value to r1 */
        //"1:\n\t"
        "LDREX      r2, [%[addr]]\n\t"      /* load value of lock */
        "CMP        r2, #1\n\t"             /* if already set */
        "STREXNE    %[res], r1, [%[addr]]\n\t" /* Sets err = 0
                                                * if store op ok */
        "CMPEQ      %[res], #0\n\t"         /* Try again if strex failed */
        "BNE        1b"
        : [res]"+r" (err)
        : [addr]"r" (lock)
        : "r1", "r2"
    );
#endif

    set_interrupt_state(s_entry);

    return err;
}

/**
 * Invalidate all caches.
 */
void cpu_invalidate_caches(void)
{
    const uint32_t rd = 0; /* Cache operation. */

    __asm__ volatile (
        "MCR     p15, 0, %[rd], c7, c7, 0\n\t"  /* Invalidate I+D caches. */
        "MCR     p15, 0, %[rd], c8, c7, 0\n\t"  /* Invalidate all I+D TLBs. */
        "MCR     p15, 0, %[rd], c7, c10, 4\n\t" /* Drain write buffer. */
        : : [rd]"r" (rd)
    );
}

/**
 * Set Context ID.
 * @param cid new Context ID.
 */
void cpu_set_cid(uint32_t cid)
{
    const int rd = 0;
    uint32_t curr_cid;

    __asm__ volatile (
        "MRC    p15, 0, %[cid], c13, c0, 1" /* Read CID */
         : [cid]"=r" (curr_cid)
    );

    if (curr_cid != cid) {
        __asm__ volatile (
            "MCR    p15, 0, %[rd], c7, c10, 4\n\t"  /* DSB */
            "MCR    p15, 0, %[cid], c13, c0, 1\n\t" /* Set CID */
            "MCR    p15, 0, %[rd], c7, c5, 0\n\t"   /* Flush I cache & BTAC */
            : : [rd]"r" (rd), [cid]"r" (cid)
        );
    }
}

/**
  * @}
  */

/**
  * @}
  */