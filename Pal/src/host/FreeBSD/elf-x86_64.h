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
 * dl-machine-x86_64.h
 *
 * This files contain architecture-specific implementation of ELF dynamic
 * relocation function.
 * The source code is imported and modified from the GNU C Library.
 */

#define ELF_MACHINE_NAME "x86_64"

#include <sys/param.h>
#include <sysdep.h>
#include <sysdeps/generic/ldsodefs.h>
#include "pal_internal.h"
/* Return the link-time address of _DYNAMIC.  Conveniently, this is the
   first element of the GOT.  This must be inlined in a function which
   uses global data.  */
static inline Elf64_Addr __attribute__ ((unused))
elf_machine_dynamic (Elf64_Addr mapbase)
{
    Elf64_Addr addr;
    
    addr = (Elf64_Addr) &_DYNAMIC + mapbase;
    
     /* This work because we have our GOT address available in the small PIC
       model.  */
    //extern const Elf64_Addr _GLOBAL_OFFSET_TABLE_[] attribute_hidden;
    //return ((Elf64_Addr)_GLOBAL_OFFSET_TABLE_);
    
    return addr;
}

/* Return the run-time load address of the shared object.  */
static inline Elf64_Addr __attribute__ ((unused))
elf_machine_load_address (void** auxv)
{
    Elf64_Addr addr = NULL, base = NULL;

    /* The easy way is just the same as on x86:
         leaq _dl_start, %0
         leaq _dl_start(%%rip), %1
         subq %0, %1
       but this does not work with binutils since we then have
       a R_X86_64_32S relocation in a shared lib.

       Instead we store the address of _dl_start in the data section
       and compare it with the current value that we can get via
       an RIP relative addressing mode.  Note that this is the address
       of _dl_start before any relocation performed at runtime.  In case
       the binary is prelinked the resulting "address" is actually a
       load offset which is zero if the binary was loaded at the address
       it is prelinked for.  */

    /* fetch environment information from aux vectors */
    //Consume envp vector
    for (; *(auxv - 1); auxv++);
    
    //Start processing auxv
    ElfW(auxv_t) *av;
    for (av = (ElfW(auxv_t) *)auxv ; av->a_type != AT_NULL ; av++)
           if (av->a_type == AT_BASE){ 
			base = av->a_un.a_val;
	   		break;}
    assert(base != NULL);
    
    /*asm ("leaq " XSTRINGIFY(_ENTRY) "(%%rip), %0\n\t"
	"subq %1, %0\n\t"
         ".section\t.data.rel.ro\n"
         "1:\t.quad " XSTRINGIFY(_ENTRY) "\n\t"
         ".previous\n\t"
         : "=r" (addr) :"r"(base) : "cc");
	*/
    return base;
}
