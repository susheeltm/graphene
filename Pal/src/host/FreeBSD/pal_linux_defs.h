/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#ifndef PAL_LINUX_DEFS_H
#define PAL_LINUX_DEFS_H

/* internal wrap native pipe inside pipe streams */
#define USE_PIPE_SYSCALL        1

#define USE_VSYSCALL_GETTIME    0
#define USE_VDSO_GETTIME        1
#define USE_CLOCK_GETTIME       1

typedef int __kernel_pid_t;
#endif /* PAL_LINUX_DEFS_H */
