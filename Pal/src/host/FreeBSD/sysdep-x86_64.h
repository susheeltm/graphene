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

/* This file is imported and modified from the GNU C Library */

#ifndef _LINUX_X86_64_SYSDEP_H
#define _LINUX_X86_64_SYSDEP_H 1

#include <sysdeps/generic/sysdep.h>
#include <libc-symbols.h>
//#include <unistd.h>
#include<sys/syscall.h>
/* For Linux we can use the system call table in the header file
	/usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#define SYS_ify(syscall_name)	__NR_##syscall_name
//BSD useds SYS_* sys call names, so we can use these to get syscall nums
#define SYS_ifyBSD(syscall_name) SYS_##syscall_name
/* This is a kludge to make syscalls.list find these under the names
   pread and pwrite, since some kernel headers define those names
   and some define the *64 names for the same system calls.  */
#if !defined __NR_pread && defined __NR_pread64
# define __NR_pread __NR_pread64
#endif
#if !defined __NR_pwrite && defined __NR_pwrite64
# define __NR_pwrite __NR_pwrite64
#endif

/* This is to help the old kernel headers where __NR_semtimedop is not
   available.  */
#ifndef __NR_semtimedop
# define __NR_semtimedop 220
#endif

#ifdef __ASSEMBLER__

/* Syntactic details of assembler.  */
#ifdef HAVE_ELF

/* ELF uses byte-counts for .align, most others use log2 of count of bytes.  */
#define ALIGNARG(log2) 1<<log2
/* For ELF we need the `.type' directive to make shared libs work right.  */
#define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg;
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name;

/* In ELF C symbols are asm symbols.  */
#undef  NO_UNDERSCORES
#define NO_UNDERSCORES

#else

#define ALIGNARG(log2) log2
#define ASM_TYPE_DIRECTIVE(name,type)   /* Nothing is specified.  */
#define ASM_SIZE_DIRECTIVE(name)        /* Nothing is specified.  */

#endif

/* Define an entry point visible from C.  */
#define ENTRY(name)                                                           \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);                                   \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)                          \
  .align ALIGNARG(4);                                                         \
  C_LABEL(name)                                                               \
  cfi_startproc;                                                              \
  CALL_MCOUNT

#undef  END
#define END(name)                                                             \
  cfi_endproc;                                                                \
  ASM_SIZE_DIRECTIVE(name)

/* If compiled for profiling, call `mcount' at the start of each function.  */
#ifdef  PROF
/* The mcount code relies on a normal frame pointer being on the stack
   to locate our caller, so push one just for its benefit.  */
#define CALL_MCOUNT                                                          \
  pushq %rbp;                                                                \
  cfi_adjust_cfa_offset(8);                                                  \
  movq %rsp, %rbp;                                                           \
  cfi_def_cfa_register(%rbp);                                                \
  call JUMPTARGET(mcount);                                                   \
  popq %rbp;                                                                 \
  cfi_def_cfa(rsp,8);
#else
#define CALL_MCOUNT             /* Do nothing.  */
#endif

#ifdef  NO_UNDERSCORES
/* Since C identifiers are not normally prefixed with an underscore
   on this system, the asm identifier `syscall_error' intrudes on the
   C name space.  Make sure we use an innocuous name.  */
#define syscall_error   __syscall_error
#define mcount          _mcount
#endif


/* Linux uses a negative return value to indicate syscall errors,
   unlike most Unices, which use the condition codes' carry flag.

   Since version 2.1 the return value of a system call might be
   negative even if the call succeeded.	 E.g., the `lseek' system call
   might return a large offset.	 Therefore we must not anymore test
   for < 0, but test for a real error by making sure the value in %eax
   is a real error number.  Linus said he will make sure the no syscall
   returns a value in -1 .. -4095 as a valid result so we can safely
   test with -4095.  */

/* We don't want the label for the error handle to be global when we define
   it here.  */
#ifdef PIC
# define SYSCALL_ERROR_LABEL 0f
#else
# define SYSCALL_ERROR_LABEL syscall_error
#endif

#undef	PSEUDO
#define	PSEUDO(name, syscall_name, args)				      \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    cmpq $-4095, %rax;							      \
    jae SYSCALL_ERROR_LABEL;						      \
  L(pseudo_end):

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  END (name)

#undef	PSEUDO_NOERRNO
#define	PSEUDO_NOERRNO(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args)

#undef	PSEUDO_END_NOERRNO
#define	PSEUDO_END_NOERRNO(name) \
  END (name)

#define ret_NOERRNO ret

#undef	PSEUDO_ERRVAL
#define	PSEUDO_ERRVAL(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    negq %rax

#undef	PSEUDO_END_ERRVAL
#define	PSEUDO_END_ERRVAL(name) \
  END (name)

#define ret_ERRVAL ret

/*
#ifndef PIC
#define SYSCALL_ERROR_HANDLER
#elif RTLD_PRIVATE_ERRNO
# define SYSCALL_ERROR_HANDLER			\
0:						\
  leaq rtld_errno(%rip), %rcx;			\
  xorl %edx, %edx;				\
  subq %rax, %rdx;				\
  movl %edx, (%rcx);				\
  orq $-1, %rax;				\
  jmp L(pseudo_end);
#elif USE___THREAD
# define SYSCALL_ERROR_ERRNO pal_errno
# define SYSCALL_ERROR_HANDLER			\
0:						\
  movq SYSCALL_ERROR_ERRNO@GOTTPOFF(%rip), %rcx;\
  xorl %edx, %edx;				\
  subq %rax, %rdx;				\
  movl %edx, %fs:(%rcx);			\
  orq $-1, %rax;				\
  jmp L(pseudo_end);
#elif defined _LIBC_REENTRANT
   Store (- %rax) into errno through the GOT.
   Note that errno occupies only 4 bytes.
# define SYSCALL_ERROR_HANDLER			\
0:						\
  xorl %edx, %edx;				\
  subq %rax, %rdx;				\
  pushq %rdx;					\
  cfi_adjust_cfa_offset(8);			\
  PUSH_ERRNO_LOCATION_RETURN;			\
  call BP_SYM (PAL(__errno_location))@PLT;		\
  POP_ERRNO_LOCATION_RETURN;			\
  popq %rdx;					\
  cfi_adjust_cfa_offset(-8);			\
  movl %edx, (%rax);				\
  orq $-1, %rax;				\
  jmp L(pseudo_end);

   A quick note: it is assumed that the call to `__errno_location' does
   not modify the stack!  
#else    Not _LIBC_REENTRANT.  
# define SYSCALL_ERROR_HANDLER			\
0:movq pal_errno@GOTPCREL(%RIP), %rcx;		\
  xorl %edx, %edx;				\
  subq %rax, %rdx;				\
  movl %edx, (%rcx);				\
  orq $-1, %rax;				\
  jmp L(pseudo_end);
#endif	   PIC
*/

/* The Linux/x86-64 kernel expects the system call parameters in
   registers according to the following table:

    syscall number	rax
    arg 1		rdi
    arg 2		rsi
    arg 3		rdx
    arg 4		r10
    arg 5		r8
    arg 6		r9

    The Linux kernel uses and destroys internally these registers:
    return address from
    syscall		rcx
    additionally clobbered: r12-r15,rbx,rbp
    eflags from syscall	r11

    Normal function call, including calls to the system call stub
    functions in the libc, get the first six parameters passed in
    registers and the seventh parameter and later on the stack.  The
    register use is as follows:

     system call number	in the DO_CALL macro
     arg 1		rdi
     arg 2		rsi
     arg 3		rdx
     arg 4		rcx
     arg 5		r8
     arg 6		r9

    We have to take care that the stack is aligned to 16 bytes.  When
    called the stack is not aligned since the return address has just
    been pushed.


    Syscalls of more than 6 arguments are not supported.  */

#undef	DO_CALL
#define DO_CALL(syscall_name, args)		\
    DOARGS_##args				\
    movl $SYS_ify (syscall_name), %eax;		\
    syscall;

#define DOARGS_0 /* nothing */
#define DOARGS_1 /* nothing */
#define DOARGS_2 /* nothing */
#define DOARGS_3 /* nothing */
#define DOARGS_4 movq %rcx, %r10;
#define DOARGS_5 DOARGS_4
#define DOARGS_6 DOARGS_5

#else	/* !__ASSEMBLER__ */
/* Define a macro which expands inline into the wrapper code for a system
   call.  */
#undef INLINE_SYSCALL

#define INLINE_SYSCALL(name, nr, args...) INTERNAL_SYSCALL(name, , nr, args)

#undef INTERNAL_SYSCALL_DECL
#define INTERNAL_SYSCALL_DECL(err) do { } while (0)

#define INTERNAL_SYSCALL_NCS(name, err, nr, args...) \
  ({						      \
    LOAD_ARGS_##nr (args)			      \
    LOAD_REGS_##nr				      \
    unsigned long resultvar;			      \
    asm volatile (		 			\
    "movq %%rax, %%r11\n\t"					\
    /*"int $3\n\t"*/		 			\
    "int $0x80\n\t"		 			\
    : "=a" (resultvar)					\
    : "0" (name) ASM_ARGS_##nr 				\
    : "memory", "cc", "r11");		      		\
    (long) IS_SYSCALL_ERROR(resultvar); 		\
   })


#undef INTERNAL_SYSCALL
#define INTERNAL_SYSCALL(name, err, nr, args...) \
INTERNAL_SYSCALL_NCS(SYS_ifyBSD(name), err, nr, ##args)  
//INTERNAL_SYSCALL_NCS (__NR_##name, err, nr, ##args)

#undef INTERNAL_SYSCALL_ERROR
#define INTERNAL_SYSCALL_ERROR(val) ((val) < 0)

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val) \
  ((unsigned long) (val) >= -4095L)

#undef INTERNAL_SYSCALL_ERRNO
#define INTERNAL_SYSCALL_ERRNO(val) (-(val))

#undef INTERNAL_SYSCALL_ERRNO_P
#define INTERNAL_SYSCALL_ERRNO_P(val) (-((long) val))

#define IS_SYSCALL_ERROR(val)				\
	({						\
    int carry;						\
    asm volatile("mov $0, %0\n\t"			\
		  "adc $0, %0\n\t"			\
		  : "=b"(carry)				\
		  :					\
		  : "cc", "memory", "eax");		\
	/* if (carry) asm volatile("int $3");	*/	\
	 (carry)? (-val):val; })
	  		

#define LOAD_ARGS_0()
#define LOAD_REGS_0					
#define ASM_ARGS_0

#define LOAD_ARGS_1(a1)					\
  long int __arg1 = (long) (a1);			\
 LOAD_ARGS_0 ()
#define LOAD_REGS_1					\
  register long int _a1 asm ("rdi") = __arg1;		\
  LOAD_REGS_0

#define ASM_ARGS_1	ASM_ARGS_0, "r" (_a1)

#define LOAD_ARGS_2(a1, a2)				\
  long int __arg2 = (long) (a2);			\
  LOAD_ARGS_1 (a1)
#define LOAD_REGS_2					\
  register long int _a2 asm ("rsi") = __arg2;		\
  LOAD_REGS_1
#define ASM_ARGS_2	ASM_ARGS_1, "r" (_a2)

#define LOAD_ARGS_3(a1, a2, a3)				\
  long int __arg3 = (long) (a3);			\
  LOAD_ARGS_2 (a1, a2)
#define LOAD_REGS_3					\
 register long int _a3 asm ("rdx") = __arg3;		\
	  LOAD_REGS_2

#define ASM_ARGS_3	ASM_ARGS_2, "r" (_a3)

#define LOAD_ARGS_4(a1, a2, a3, a4)			\
  long int __arg4 = (long) (a4);			\
  LOAD_ARGS_3 (a1, a2, a3)
#define LOAD_REGS_4					\
 register long int _a4 asm ("rcx") = __arg4;		\
	  LOAD_REGS_3
#define ASM_ARGS_4	ASM_ARGS_3, "r" (_a4)

#define LOAD_ARGS_5(a1, a2, a3, a4, a5)			\
  long int __arg5 = (long) (a5);			\
  LOAD_ARGS_4 (a1, a2, a3, a4)
#define LOAD_REGS_5					\
	register long int _a5 asm ("r8") = __arg5;		\
  LOAD_REGS_4
#define ASM_ARGS_5	ASM_ARGS_4, "r" (_a5)

#define LOAD_ARGS_6(a1, a2, a3, a4, a5, a6)		\
  long int __arg6 = (long) (a6);			\
	  LOAD_ARGS_5 (a1, a2, a3, a4, a5)
#define LOAD_REGS_6					\
register long int _a6 asm ("r9") = __arg6;		\
  LOAD_REGS_5
#define ASM_ARGS_6	ASM_ARGS_5, "r" (_a6)

#endif	/* __ASSEMBLER__ */


/* Pointer mangling support.  */
#if defined NOT_IN_libc && defined IS_IN_rtld
/* We cannot use the thread descriptor because in ld.so we use setjmp
   earlier than the descriptor is initialized.  */
# ifdef __ASSEMBLER__
#  define PTR_MANGLE(reg)	xorq __pointer_chk_guard_local(%rip), reg
#  define PTR_DEMANGLE(reg)	PTR_MANGLE (reg)
# else
#  define PTR_MANGLE(reg)	asm ("xorq __pointer_chk_guard_local(%%rip), %0"\
				     : "=r" (reg) : "0" (reg))
#  define PTR_DEMANGLE(reg)	PTR_MANGLE (reg)
# endif
#else
# ifdef __ASSEMBLER__
#  define PTR_MANGLE(reg)	xorq %fs:POINTER_GUARD, reg
#  define PTR_DEMANGLE(reg)	PTR_MANGLE (reg)
# else
#  define PTR_MANGLE(var)	asm ("xorq %%fs:%c2, %0"		      \
				     : "=r" (var)			      \
				     : "0" (var),			      \
				       "i" (offsetof (tcbhead_t,	      \
						      pointer_guard)))
#  define PTR_DEMANGLE(var)	PTR_MANGLE (var)
# endif
#endif

#endif /* linux/x86_64/sysdep.h */
