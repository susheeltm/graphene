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
 * pal.h
 *
 * This file contains definition of PAL host ABI.
 */

#ifndef PAL_H
#define PAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __x86_64__
typedef unsigned long PAL_NUM;
#endif
typedef const char *  PAL_STR;
typedef const void *  PAL_PTR;
typedef void *        PAL_BUF;
typedef unsigned int  PAL_FLG;
typedef unsigned int  PAL_IDX;
typedef char          PAL_CHR;
typedef bool          PAL_BOL;

typedef struct {
#ifdef __x86_64__
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15, rdi, rsi, rbp, rbx, rdx, rax,
             rcx, rsp, rip, efl, csgsfs, err, trapno, oldmask, cr2;
#endif
} PAL_CONTEXT;

#define PAL_TRUE  true
#define PAL_FALSE false

/********** PAL TYPE DEFINITIONS **********/
enum {
    pal_type_none = 0,
    pal_type_file,
    pal_type_pipe,
    pal_type_pipesrv,
    pal_type_pipecli,
    pal_type_pipeprv,
    pal_type_dev,
    pal_type_dir,
    pal_type_tcp,
    pal_type_tcpsrv,
    pal_type_udp,
    pal_type_udpsrv,
    pal_type_process,
    pal_type_mcast,
    pal_type_thread,
    pal_type_semaphore,
    pal_type_event,
    pal_type_gipc,
    PAL_HANDLE_TYPE_BOUND,
};

#ifdef IN_PAL
struct atomic_int {
    volatile int32_t counter;
} __attribute__((aligned(sizeof(int))));

typedef struct atomic_int PAL_REF;

typedef struct {
    PAL_IDX type;
    PAL_REF ref;
    PAL_FLG flags;
} PAL_HDR;

/* internal Mutex design, the structure has to align at integer boundary
   because it is required by futex call. If DEBUG_MUTEX is defined,
   mutex_handle will record the owner of mutex locking. */
struct mutex_handle {
    struct atomic_int value;
#ifdef DEBUG_MUTEX
    int owner;
#endif
};

# include "pal_host.h"

# define SET_HANDLE_TYPE(handle, t)             \
    do {                                        \
        (handle)->__in.type = pal_type_##t;     \
        (handle)->__in.ref.counter = 0;         \
        (handle)->__in.flags = 0;               \
    } while (0)

# define IS_HANDLE_TYPE(handle, t)              \
    ((handle)->__in.type == pal_type_##t)

#else
typedef union pal_handle
{
    struct {
        PAL_IDX type;
    } __in;
} * PAL_HANDLE;
#endif /* !IN_PAL */

/* PAL identifier poison value */
#define PAL_IDX_POISON          ((PAL_IDX) -1)
#define __PAL_GET_TYPE(h)       ((h)->__in.type)
#define __PAL_CHECK_TYPE(h, t)  (__PAL_GET_TYPE(h) == pal_type_##t)

/********** PAL APIs **********/
typedef struct {
    /* program manifest */
    PAL_HANDLE manifest_handle;
    /* string for executable name */
    PAL_STR executable;
    /* address where executable is loaded */
    PAL_BUF executable_begin;
    PAL_BUF executable_end;
    /* address where PAL is loaded */
    PAL_BUF library_begin;
    PAL_BUF library_end;
    /* host page size */
    PAL_NUM pagesize;
    /* host allocation alignment */
    PAL_NUM alloc_align;
    /* handle of parent process */
    PAL_HANDLE parent_process;
    /* handle of first thread */
    PAL_HANDLE first_thread;
    /* debug stream */
    PAL_HANDLE debug_stream;
    /* broadcast RPC stream */
    PAL_HANDLE broadcast_stream;
} PAL_CONTROL;

#define pal_control (*pal_control_addr())

extern PAL_CONTROL * pal_control_addr (void);

/* The ABI includes three calls to allocate, free, and modify the
 * permission bits on page-base virtual memory. Permissions in-
 * clude read, write, execute, and guard. Memory regions can be
 * unallocated, reserved, or backed by committed memory
 */

/* Memory Allocation Flags */
#define PAL_ALLOC_32BIT       0x0001   /* Only give out 32-bit addresses */
#define PAL_ALLOC_RESERVE     0x0002   /* Only reserve the memory */

/* Memory Protection Flags */
#define PAL_PROT_NONE       0x0     /* 0x0 Page can not be accessed. */
#define PAL_PROT_READ       0x1     /* 0x1 Page can be read. */
#define PAL_PROT_WRITE      0x2     /* 0x2 Page can be written. */
#define PAL_PROT_EXEC       0x4     /* 0x4 Page can be executed. */
#define PAL_PROT_WRITECOPY  0x8     /* 0x8 Copy on write */


PAL_BUF
DkVirtualMemoryAlloc (PAL_BUF addr, PAL_NUM size, PAL_FLG alloc_type,
                      PAL_FLG prot);

void
DkVirtualMemoryFree (PAL_BUF addr, PAL_NUM size);

PAL_BOL
DkVirtualMemoryProtect (PAL_BUF addr, PAL_NUM size,
                        PAL_FLG prot);


/* The ABI includes one call to create a child process and one call to
 * terminate the running process. A child process does not inherit
 * any objects or memory from its parent process and the parent
 * process may not modify the execution of its children. A parent can
 * wait for a child to exit using its handle. Parent and child may
 * communicate through I/O streams provided by the parent to the
 * child at creation
 */

#define PAL_PROCESS_MASK         0x0

PAL_HANDLE
DkProcessCreate (PAL_STR uri, PAL_FLG flags, PAL_STR * args);

void
DkProcessExit (PAL_NUM exitCode);

#define PAL_SANDBOX_PIPE         0x1

PAL_BOL
DkProcessSandboxCreate (PAL_STR manifest, PAL_FLG flags);

/* The stream ABI includes nine calls to open, read, write, map, unmap, 
 * truncate, flush, delete and wait for I/O streams and three calls to 
 * access metadata about an I/O stream. The ABI purposefully does not
 * provide an ioctl call. Supported URI schemes include file:, pipe:,
 * http:, https:, tcp:, udp:, pipe.srv:, http.srv, tcp.srv:, and udp.srv:.
 * The latter four schemes are used to open inbound I/O streams for
 * server applications.
 */

/* DkStreamOpen
 *   access_mode: WRONLY or RDONLY or RDWR
 *   share_flags: permission for the created file
 *   create_flags: the creation options for the file
 *   options: other options
 */

/* Stream Access Flags */
#define PAL_ACCESS_RDONLY   00
#define PAL_ACCESS_WRONLY   01
#define PAL_ACCESS_RDWR     02
#define PAL_ACCESS_MASK     03

/* Stream Sharing Flags */
#define PAL_SHARE_GLOBAL_X    01
#define PAL_SHARE_GLOBAL_W    02
#define PAL_SHARE_GLOBAL_R    04
#define PAL_SHARE_GROUP_X    010
#define PAL_SHARE_GROUP_W    020
#define PAL_SHARE_GROUP_R    040
#define PAL_SHARE_OWNER_X   0100
#define PAL_SHARE_OWNER_W   0200
#define PAL_SHARE_OWNER_R   0400
#define PAL_SHARE_MASK      0777

/* Stream Create Flags */
#ifdef __linux__
#define PAL_CREAT_TRY        0100       /* 0100 Create file if file not
                                           exist (O_CREAT) */
#else
#define PAL_CREAT_TRY        0200       /* 0100 Create file if file not
                                           exist (O_CREAT) */
#endif

#define PAL_CREAT_ALWAYS     0200       /* 0300 Create file and fail if file
                                           already exist (O_CREAT|O_EXCL) */
#define PAL_CREAT_MASK       0300

/* Stream Option Flags */
#define PAL_OPTION_NONBLOCK     04000
#define PAL_OPTION_MASK         04000

PAL_HANDLE
DkStreamOpen (PAL_STR uri, PAL_FLG access, PAL_FLG share_flags,
              PAL_FLG create, PAL_FLG options);

PAL_HANDLE
DkStreamWaitForClient (PAL_HANDLE handle);

PAL_NUM
DkStreamRead (PAL_HANDLE handle, PAL_NUM offset, PAL_NUM count,
              PAL_BUF buffer, PAL_BUF source, PAL_NUM size);

PAL_NUM
DkStreamWrite (PAL_HANDLE handle, PAL_NUM offset, PAL_NUM count,
               PAL_PTR buffer, PAL_STR dest);

#define PAL_DELETE_RD       01
#define PAL_DELETE_WR       02

void
DkStreamDelete (PAL_HANDLE handle, PAL_FLG access);

PAL_BUF
DkStreamMap (PAL_HANDLE handle, PAL_BUF address, PAL_FLG prot,
             PAL_NUM offset, PAL_NUM size);

void
DkStreamUnmap (PAL_BUF addr, PAL_NUM size);

PAL_NUM
DkStreamSetLength (PAL_HANDLE handle, PAL_NUM length);

PAL_BOL
DkStreamFlush (PAL_HANDLE handle);

PAL_BOL
DkSendHandle (PAL_HANDLE handle, PAL_HANDLE cargo);

PAL_HANDLE
DkReceiveHandle (PAL_HANDLE handle);

/* stream attribute structure */
typedef struct {
    PAL_IDX type;
    PAL_NUM file_id;
    PAL_NUM size;
    PAL_NUM access_time;
    PAL_NUM change_time;
    PAL_NUM create_time;
    PAL_BOL disconnected;
    PAL_BOL readable;
    PAL_BOL writeable;
    PAL_BOL runnable;
    PAL_FLG share_flags;
    PAL_BOL nonblocking;
    PAL_BOL reuseaddr;
    PAL_NUM linger;
    PAL_NUM receivebuf;
    PAL_NUM sendbuf;
    PAL_NUM receivetimeout;
    PAL_NUM sendtimeout;
    PAL_BOL tcp_cork;
    PAL_BOL tcp_keepalive;
    PAL_BOL tcp_nodelay;
} PAL_STREAM_ATTR;

PAL_BOL
DkStreamAttributesQuery (PAL_STR uri, PAL_STREAM_ATTR * attr);

PAL_BOL
DkStreamAttributesQuerybyHandle (PAL_HANDLE handle,
                                 PAL_STREAM_ATTR * attr);

PAL_BOL
DkStreamAttributesSetbyHandle (PAL_HANDLE handle, PAL_STREAM_ATTR * attr);

PAL_NUM
DkStreamGetName (PAL_HANDLE handle, PAL_BUF buffer, PAL_NUM size);

PAL_BOL
DkStreamChangeName (PAL_HANDLE handle, PAL_STR uri);

/* The ABI supports multithreading through five calls to create,
 * sleep, yield the scheduler quantum for, resume execution of, and
 * terminate threads, as well as seven calls to create, signal, and
 * block on synchronization objects
 */

#define PAL_THREAD_PARENT       0x00008000  /* Have the same parent as cloner */
#define PAL_THREAD_DETACHED     0x00400000  /* Thread deattached */
#define PAL_THREAD_STOPPED      0x02000000  /* Start in stopped state */

#define PAL_THREAD_MASK         0x02408000

PAL_HANDLE
DkThreadCreate (PAL_PTR addr, PAL_PTR param, PAL_FLG flags);

PAL_BUF
DkThreadPrivate (PAL_BUF addr);

// assuming duration to be in microseconds
PAL_NUM
DkThreadDelayExecution (PAL_NUM duration);

void
DkThreadYieldExecution (void);

void
DkThreadExit (void);

PAL_BOL
DkThreadResume (PAL_HANDLE thread);

/* Exception Handling */
/* Div-by-zero */
#define PAL_EVENT_DIVZERO       1
/* segmentation fault, protection fault, bus fault */
#define PAL_EVENT_MEMFAULT      2
/* illegal instructions */
#define PAL_EVENT_ILLEGAL       3
/* terminated by external program */
#define PAL_EVENT_QUIT          4
/* suspended by external program */
#define PAL_EVENT_SUSPEND       5
/* continued by external program */
#define PAL_EVENT_RESUME        6
/* failure within PAL calls */
#define PAL_EVENT_FAILURE       7

#define PAL_EVENT_NUM_BOUND     8

#define PAL_EVENT_PRIVATE      0x0001       /* upcall specific to thread */
#define PAL_EVENT_RESET        0x0002       /* reset the event upcall */

PAL_BOL
DkSetExceptionHandler (void (*handler) (PAL_PTR event, PAL_NUM arg,
                                        PAL_CONTEXT * context),
                       PAL_NUM event, PAL_FLG flags);

void
DkExceptionReturn (PAL_PTR event);


/* parameter: keeping int threadHandle for now (to be in sync with the paper).
 * We may want to replace it with a PAL_HANDLE. Ideally, either use PAL_HANDLE
 * or threadHandle.
 */

PAL_HANDLE
DkSemaphoreCreate (PAL_NUM initialCount, PAL_NUM maxCount);

/* DkSemaphoreDestroy deprecated, replaced by DkObjectClose */

/* TSAI: I preserve this API because DkObjectsWaitAny can't acquire multiple
 * counts of a semaphore. Acquiring multiple counts is required for
 * implementing a read-write-lock. To make this API complementary to
 * DkObjectsWaitAny, I added a 'timeout' to its arguments */

/* DkSemaphoreAcquire deprecated */

void
DkSemaphoreRelease (PAL_HANDLE semaphoreHandle, PAL_NUM count);

/* DkSemaphoreGetCurrentCount deprecated */

PAL_HANDLE
DkNotificationEventCreate (PAL_BOL initialState);

PAL_HANDLE
DkSynchronizationEventCreate (PAL_BOL initialState);

/* DkEventDestroy deprecated, replaced by DkObjectClose */

void
DkEventSet (PAL_HANDLE eventHandle);

/* DkEventWait deprecated, replaced by DkObjectsWaitAny */

void
DkEventClear (PAL_HANDLE eventHandle);

#define NO_TIMEOUT      ((PAL_NUM) -1)

/* assuming timeout to be in microseconds */
PAL_HANDLE
DkObjectsWaitAny (PAL_NUM count, PAL_HANDLE * handleArray, PAL_NUM timeout);

/* the ABI includes seven assorted calls to get wall clock
 * time, generate cryptographically-strong random bits, flush por-
 * tions of instruction caches, increment and decrement the reference
 * counts on objects shared between threads, and to coordinate
 * threads with the security monitor during process serialization
 */

/* assuming the time to be in microseconds */
PAL_NUM
DkSystemTimeQuery (void);

PAL_NUM
DkRandomBitsRead (PAL_BUF buffer, PAL_NUM size);

void
DkObjectClose (PAL_HANDLE objectHandle);

PAL_BOL
DkInstructionCacheFlush (PAL_PTR addr, PAL_NUM size);

PAL_HANDLE
DkCreatePhysicalMemoryChannel (PAL_NUM * key);

PAL_NUM
DkPhysicalMemoryCommit (PAL_HANDLE channel, PAL_NUM entries, PAL_BUF * addrs,
                        PAL_NUM * sizes, PAL_FLG flags);

PAL_NUM
DkPhysicalMemoryMap (PAL_HANDLE channel, PAL_NUM entries, PAL_BUF * addrs,
                     PAL_NUM * sizes, PAL_FLG * prots);

#endif /* PAL_H */
