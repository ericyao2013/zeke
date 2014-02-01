/**
 *******************************************************************************
 * @file    syscall.c
 * @author  Olli Vanhoja
 *
 * @brief   Kernel's internal Syscall handler that is called from kernel scope.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <sched.h>
#define KERNEL_INTERNAL 1
#ifndef PU_TEST_BUILD
#include <hal/hal_core.h>
#endif
#include <errno.h>
#include <syscall.h>

/* For all Syscall groups */
#if PU_TEST_BUILD == 0
#define FOR_ALL_SYSCALL_GROUPS(apply)                       \
    apply(SYSCALL_GROUP_SCHED, sched_syscall)               \
    apply(SYSCALL_GROUP_SCHED_THREAD, sched_syscall_thread) \
    apply(SYSCALL_GROUP_SIGNAL, ksignal_syscall)            \
    apply(SYSCALL_GROUP_FS, fs_syscall)                     \
    apply(SYSCALL_GROUP_LOCKS, ulocks_syscall)
#else
#define FOR_ALL_SYSCALL_GROUPS(apply)                       \
    apply(SYSCALL_GROUP_SCHED, sched_syscall)               \
    apply(SYSCALL_GROUP_SCHED_THREAD, sched_syscall_thread) \
    apply(SYSCALL_GROUP_SIGNAL, ksignal_syscall)
#endif

/*
 * Declare prototypes of syscall handlers.
 */
#define DECLARE_SCHANDLER(major, function) extern uint32_t function(uint32_t type, void * p);
FOR_ALL_SYSCALL_GROUPS(DECLARE_SCHANDLER)
#undef DECLARE_SCHANDLER

typedef uint32_t (*kernel_syscall_handler_t)(uint32_t type,  void * p);

static kernel_syscall_handler_t syscall_callmap[] = {
    #define SYSCALL_MAP_X(major, function) [major] = function,
    FOR_ALL_SYSCALL_GROUPS(SYSCALL_MAP_X)
    #undef SYSCALL_MAP_X
};

/**
 * Kernel's internal Syscall handler/translator
 *
 * This function is called from interrupt handler. This function calls the
 * actual kernel function and returns a result pointer/data to the interrupt
 * handler which returns it to the original caller, which is usually a library
 * function in kernel.c
 * @param type  syscall type.
 * @param p     pointer to the parameter or parameter structure.
 * @return      result value or pointer to the result from the called kernel
 *              function.
 */
uint32_t _intSyscall_handler(uint32_t type, void * p)
{
    kernel_syscall_handler_t fpt;
    uint32_t major = SYSCALL_MAJOR(type);

    if (major >= (sizeof(syscall_callmap) / sizeof(void *))) {
        current_thread->errno = ENOSYS; /* Not supported. */
        /* 0/NULL means usually ERROR, however there is some cases where NULL as
         * a return value doesn't necessarily mean error. */
        return 0; /* NULL */
    }

    current_thread->errno = 0; /* Default value for errno */
    fpt = syscall_callmap[major];
    {
        uint32_t retval;
        retval = fpt(type, p);
        eval_syscall_block(); /* This will set mach dep block flag to 1 if
                               * was blocking. Eg. in ARM11 R1 is set to 0/1.
                               */
        return retval;
    }
}
