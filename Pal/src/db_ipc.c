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
 * db_ipc.c
 *
 * This file contains APIs for physical memory bulk copy across processes.
 */

#include "pal_defs.h"
#include "pal.h"
#include "pal_internal.h"
#include "pal_error.h"
#include "api.h"

PAL_HANDLE
DkCreatePhysicalMemoryChannel (PAL_NUM * key)
{
    store_frame(CreatePhysicalMemoryChannel);

    PAL_HANDLE handle = NULL;
    int ret = _DkCreatePhysicalMemoryChannel(&handle, key);
    if (ret < 0) {
        notify_failure(-ret);
        return NULL;
    }

    return handle;
}

PAL_NUM
DkPhysicalMemoryCommit (PAL_HANDLE channel, PAL_NUM entries, PAL_BUF * addrs,
                        PAL_NUM * sizes, PAL_FLG flags)
{
    store_frame(PhysicalMemoryCommit);

    if (!addrs || !sizes || !channel ||
        !IS_HANDLE_TYPE(channel, gipc)) {
        notify_failure(PAL_ERROR_INVAL);
        return 0;
    }

    int ret = _DkPhysicalMemoryCommit(channel, entries, addrs, sizes, flags);

    if (ret < 0) {
        notify_failure(-ret);
        return 0;
    }

    return ret;
}

PAL_NUM
DkPhysicalMemoryMap (PAL_HANDLE channel, PAL_NUM entries, PAL_BUF * addrs,
                     PAL_NUM * sizes, PAL_FLG * prots)
{
    store_frame(PhysicalMemoryMap);

    if (!sizes || !channel || !IS_HANDLE_TYPE(channel, gipc)) {
        notify_failure(PAL_ERROR_INVAL);
        return 0;
    }

    int ret = _DkPhysicalMemoryMap(channel, entries, addrs, sizes, prots);

    if (ret < 0) {
        notify_failure(-ret);
        return 0;
    }

    return ret;
}
