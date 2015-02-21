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
 * pal_debug.h
 *
 * This file contains definitions of APIs used for debug purposes.
 */

#ifndef PAL_DEBUG_H
#define PAL_DEBUG_H
//#define __manual_debug__
#include "pal.h"

#ifdef IN_PAL

void __assert (void);

#define assert(val)                                                      \
    do {                                                                 \
        if (!(val)) {                                                    \
            printf("Assertion failed (%s): %s: %d\n", #val,              \
                   __FILE__, __LINE__);                                  \
            __assert();                                                  \
            _DkProcessExit(1);                                           \
        }                                                                \
    } while (0)

#define abort(msg)                                                       \
    do {                                                                 \
        if (!(val)) {                                                    \
            printf("Assertion failed (%s): %s: %d\n", msg,               \
                   __FILE__, __LINE__);                                  \
            __assert();                                                  \
            _DkProcessExit(1);                                           \
        }                                                                \
    } while (0)

#else

int pal_printf  (const char *fmt, ...);
int pal_snprintf (char *buf, size_t n, const char *fmt, ...);
int pal_atoi (const char *nptr);
long int pal_atol (const char *nptr);

void DkDebugAttachBinary (PAL_STR uri, PAL_PTR start_addr);
void DkDebugDetachBinary (PAL_PTR start_addr);

#endif

#endif
