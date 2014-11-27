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
 * db_rtld.c
 *
 * This file contains utilities to load ELF binaries into the memory
 * and link them against each other.
 * The source code in this file is imported and modified from the GNU C
 * Library.
 */

#include "pal_defs.h"
#include "pal_linux_defs.h"
#include "pal.h"
#include "pal_internal.h"
#include "pal_linux.h"
#include "pal_debug.h"
#include "pal_error.h"
#include "pal_security.h"
#include "pal_rtld.h"
#include "api.h"

#include <sysdeps/generic/ldsodefs.h>
#include <elf/elf.h>
#include <bits/dlfcn.h>

#include "elf-x86_64.h"

struct link_map * rtld_map = NULL;

extern void setup_elf_hash (struct link_map *map);

void setup_pal_map (const char * realname, ElfW(Dyn) ** dyn, ElfW(Addr) addr)
{
    pal_printf("Loaded libraries %x", &loaded_libraries);
    assert (loaded_libraries == NULL || loaded_libraries == 0x0); //-> points to a bad address. Why?
	
    const ElfW(Ehdr) * header = (void *) addr;
    struct link_map * l = new_elf_object(realname, OBJECT_RTLD);
    memcpy(l->l_info, dyn, sizeof(l->l_info));
    l->l_real_ld = l->l_ld = (void *) elf_machine_dynamic(addr);
    l->l_addr  = addr;
    l->l_entry = header->e_entry;
    l->l_phdr  = (void *) (addr + header->e_phoff);
    l->l_phnum = header->e_phnum;
    l->l_relocated = true;
    l->l_lookup_symbol = true;
    l->l_soname = "libpal.so";
    l->l_text_start = (ElfW(Addr)) &text_start;
    l->l_text_end   = (ElfW(Addr)) &text_end;
    l->l_data_start = (ElfW(Addr)) &data_start;
    l->l_data_end   = (ElfW(Addr)) &data_end;
    setup_elf_hash(l);

    void * begin_hole = (void *) ALLOC_ALIGNUP(l->l_text_end);
    void * end_hole = (void *) ALLOC_ALIGNDOWN(l->l_data_start);

    /* Usually the test segment and data segment of a loaded library has
       a gap between them. Need to fill the hole with a empty area */
    if (begin_hole < end_hole) {
        void * addr = begin_hole;
        _DkVirtualMemoryAlloc(&addr, end_hole - begin_hole,
                              PAL_ALLOC_RESERVE, PAL_PROT_NONE);
    }

    /* Set up debugging before the debugger is notified for the first time.  */
    if (l->l_info[DT_DEBUG] != NULL)
        l->l_info[DT_DEBUG]->d_un.d_ptr = (ElfW(Addr)) &pal_r_debug;

    l->l_prev = l->l_next = NULL;
    rtld_map = l;
    loaded_libraries = l;

    if (!pal_sec_info._r_debug) {
        pal_r_debug.r_version = 1;
        pal_r_debug.r_brk = (ElfW(Addr)) &pal_dl_debug_state;
        pal_r_debug.r_ldbase = addr;
        pal_r_debug.r_map = loaded_libraries;
        pal_sec_info._r_debug = &pal_r_debug;
        pal_sec_info._dl_debug_state = &pal_dl_debug_state;
    } else {
        pal_sec_info._r_debug->r_state = RT_ADD;
        pal_sec_info._dl_debug_state();

        if (pal_sec_info._r_debug->r_map) {
            l->l_prev = pal_sec_info._r_debug->r_map;
            pal_sec_info._r_debug->r_map->l_next = l;
        } else {
            pal_sec_info._r_debug->r_map = loaded_libraries;
        }

        pal_sec_info._r_debug->r_state = RT_CONSISTENT;
        pal_sec_info._dl_debug_state();
    }
}

#if USE_VDSO_GETTIME == 1
void setup_vdso_map (ElfW(Addr) addr)
{
    const ElfW(Ehdr) * header = (void *) addr;
    struct link_map * l = new_elf_object("vdso", OBJECT_RTLD);

    l->l_addr  = addr;
    l->l_entry = header->e_entry;
    l->l_phdr  = (void *) (addr + header->e_phoff);
    l->l_phnum = header->e_phnum;
    l->l_relocated = true;
    l->l_soname = "libpal.so";

    ElfW(Addr) load_offset = 0;
    const ElfW(Phdr) * ph;
    for (ph = l->l_phdr; ph < &l->l_phdr[l->l_phnum]; ++ph)
        switch (ph->p_type) {
            case PT_LOAD:
                load_offset = addr + (ElfW(Addr)) ph->p_offset
                              - (ElfW(Addr)) ph->p_vaddr;
                break;
            case PT_DYNAMIC:
                l->l_real_ld = l->l_ld = (void *) addr + ph->p_offset;
                l->l_ldnum = ph->p_memsz / sizeof (ElfW(Dyn));
                break;
        }

    ElfW(Dyn) local_dyn[4];
    int ndyn = 0;
    ElfW(Dyn) * dyn;
    for (dyn = l->l_ld ; dyn < &l->l_ld[l->l_ldnum]; ++dyn)
        switch(dyn->d_tag) {
            case DT_STRTAB:
            case DT_SYMTAB:
                local_dyn[ndyn] = *dyn;
                local_dyn[ndyn].d_un.d_ptr += load_offset;
                l->l_info[dyn->d_tag] = &local_dyn[ndyn++];
                break;
            case DT_HASH: {
                ElfW(Word) * h = (ElfW(Word) *) (D_PTR(dyn) + load_offset);
                l->l_nbuckets = h[0];
                l->l_buckets  = &h[2];
                l->l_chain    = &h[l->l_nbuckets + 2];
                break;
            }
            case DT_VERSYM:
            case DT_VERDEF:
                local_dyn[ndyn] = *dyn;
                local_dyn[ndyn].d_un.d_ptr += load_offset;
                l->l_info[VERSYMIDX(dyn->d_tag)] = &local_dyn[ndyn++];
                break;
        }

#if USE_CLOCK_GETTIME == 1
    const char * gettime = "__vdso_clock_gettime";
#else
    const char * gettime = "__vdso_gettimeofday";
#endif
    uint_fast32_t fast_hash = elf_fast_hash(gettime);
    long int hash = elf_hash(gettime);
    ElfW(Sym) * sym = NULL;

    sym = do_lookup_map(NULL, gettime, fast_hash, hash, l);
    if (sym)
#if USE_CLOCK_GETTIME == 1
        __vdso_clock_gettime =
#else
        __vdso_gettimeofday =
#endif
                (void *) (load_offset + sym->st_value);

    free(l);
}
#endif

ElfW(Addr) resolve_map_in_rtld (ElfW(Sym) * ref)
{
    /* We are not using this, because in Linux we can rely on
       rtld_map to directly lookup symbols */
    return 0;
}
