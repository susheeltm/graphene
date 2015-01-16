/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

/* Copyright (C) 2014 OSCAR lab, Stony Brook University
   This file is part of Graphene Library OS.

   Graphene Library OS is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Graphene Library OS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/*
 * db_threading.c
 *
 * This file contain APIs to create, exit and yield a thread.
 */

#include "pal_defs.h"
#include "pal_linux_defs.h"
#include "pal.h"
#include "pal_internal.h"
#include "pal_linux.h"
#include "pal_error.h"
#include "pal_debug.h"
#include "api.h"

#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/timespec.h>
#include <unistd.h>
#include <pthread.h>/* default size of a new thread */
#define PAL_THREAD_STACK_SIZE allocsize

/* _DkThreadCreate for internal use. Create an internal thread
   inside the current process. The arguments callback and param
   specify the starting function and parameters */
int _DkThreadCreate (PAL_HANDLE * handle, int (*callback) (void *),
                     const void * param, int flags)
{
   void * child_stack = NULL;

    if (_DkVirtualMemoryAlloc(&child_stack,
                              PAL_THREAD_STACK_SIZE, 0,
                              PAL_PROT_READ|PAL_PROT_WRITE) < 0)
        return -PAL_ERROR_NOMEM;

    // move child_stack to the top of stack. 
    child_stack += PAL_THREAD_STACK_SIZE;

    // align child_stack to 16 
    child_stack = (void *) ((uintptr_t) child_stack & ~16);

    flags &= PAL_THREAD_MASK;
	
    printf("",param);//Just to stop the compiler from optimizing out param!!
    //int tid = 0;
    //int ret = INLINE_SYSCALL(fork,0);
    int ret = rfork_thread(
		    RFPROC|RFFDG|RFSIGSHARE|RFMEM, 
		    child_stack, 
		    callback, 
		    (void *)param);
    //int ret = INLINE_SYSCALL(rfork,1, RFPROC|RFFDG|RFSIGSHARE|RFNOWAIT);
    /*if(ret == 0)
    {
	int r = ((int (*) (const void *))callback) (param);
	_DkThreadExit(r);

    }
    tid = ret;
	*/
    /*Clone does not exist in BSD, need to change to Pthread if we want this facility
    int ret = __clone(callback, child_stack,
                      CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SYSVSEM|
                      CLONE_THREAD|CLONE_SIGHAND|CLONE_PTRACE|
                      CLONE_PARENT_SETTID|flags,
                      param, &tid, NULL);

    */
    if (IS_ERR(ret))
        return -PAL_ERROR_DENIED;
    
    PAL_HANDLE hdl = malloc(HANDLE_SIZE(thread));
    SET_HANDLE_TYPE(hdl, thread);
    hdl->thread.tid = ret;
    *handle = hdl;

    /* _DkThreadAdd(tid); */

    return 0;
}

#if defined(__i386__)
#include <asm/ldt.h>
#else
#include <machine/sysarch.h> 
#endif

/* PAL call DkThreadPrivate: set up the thread private area for the
   current thread. if addr is 0, return the current thread private
   area. */
void * _DkThreadPrivate (void * addr)
{
    //No arch_set_fs etc in BSD, Linux specific only.
    // return NULL;
    
    int ret = 0;

    if ((void *) addr == 0) {
// 32-Bit Not touched a bit...!! 
// Changed only the 64-Bit BSD part.!
#if defined(__i386__)
        struct user_desc u_info;

        ret = INLINE_SYSCALL(get_thread_area, 1, &u_info);

        if (IS_ERR(ret))
            return NULL;

        void * thread_area = u_info->base_addr;
#else
        unsigned long ret_addr;

        ret = INLINE_SYSCALL(sysarch, 2, AMD64_GET_FSBASE, &ret_addr);

        if (IS_ERR(ret))
            return NULL;

        void * thread_area = (void *) ret_addr;
#endif
        return thread_area;
    } else {
#if defined(__i386__)
        struct user_desc u_info;

        ret = INLINE_SYSCALL(get_thread_area, 1, &u_info);

        if (IS_ERR(ret))
            return NULL;

        u_info->entry_number = -1;
        u_info->base_addr = (unsigned int) addr;

        ret = INLINE_SYSCALL(set_thread_area, 1, &u_info);
#else
        ret = INLINE_SYSCALL(sysarch, 2, AMD64_SET_FSBASE, addr);
#endif
        if (IS_ERR(ret))
            return NULL;

        return addr;
    }
}

int _DkThreadDelayExecution (unsigned long * duration)
{
    struct timespec sleeptime;
    struct timespec remainingtime;

    long sec = (unsigned long) *duration / 1000000;
    long microsec = (unsigned long) *duration - (sec * 1000000);

    sleeptime.tv_sec = sec;
    sleeptime.tv_nsec = microsec * 1000;

    int ret = INLINE_SYSCALL(nanosleep, 2, &sleeptime, &remainingtime);

    if (IS_ERR(ret)) {
        PAL_NUM remaining = remainingtime.tv_sec * 1000000 +
                            remainingtime.tv_nsec / 1000;

        *duration -= remaining;
        return -PAL_ERROR_INTERRUPTED;
    }

    return 0;
}

/* PAL call DkThreadYieldExecution. Yield the execution
   of the current thread. */
void _DkThreadYieldExecution (void)
{
    INLINE_SYSCALL(sched_yield, 0);
}

/* _DkThreadExit for internal use: Thread exiting */
void _DkThreadExit (int exitcode)
{
    INLINE_SYSCALL(exit, 1, 0);
}

int _DkThreadResume (PAL_HANDLE threadHandle)
{
    int ret = INLINE_SYSCALL(thr_kill2, 3, pal_linux_config.pid,\
                             threadHandle->thread.tid, SIGCONT);//BSD specific syscall
    //int ret = INLINE_SYSCALL(kill, 2, threadHandle->thread.tid, SIGCONT);
    if (IS_ERR(ret))
        return -PAL_ERROR_DENIED;

    return 0;
}

struct handle_ops thread_ops = {
    /* nothing */
};
