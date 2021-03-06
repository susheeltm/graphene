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
 * pal_host.h
 *
 * This file contains definition of PAL host ABI.
 */

#ifndef PAL_HOST_H
#define PAL_HOST_H

#ifndef IN_PAL
# error "cannot be included outside PAL"
#endif

typedef struct {
    PAL_IDX type;
    PAL_REF refcount;
} PAL_HDR;

typedef union pal_handle
{
    /* TSAI: Here we define the internal types of PAL_HANDLE
     * in PAL design, user has not to access the content inside the
     * handle, also there is no need to allocate the internal
     * handles, so we hide the type name of these handles on purpose.
     */

    struct {
        PAL_IDX type;
        PAL_REF refcount;
        PAL_IDX fd;
    } __in;

    struct {
        PAL_HDR __in;
    } file;

    struct {
        PAL_HDR __in;

    struct {
        PAL_HDR __in;
    } pipeprv;

    struct {
        PAL_HDR __in;
    } dev;

    struct {
        PAL_HDR __in;
    } dir;

    struct {
        PAL_HDR __in;
    } gipc;

    struct {
        PAL_HDR __in;
    } sock;

    struct {
        PAL_HDR __in;
    } process;

    struct {
        PAL_HDR __in;
    } mcast;

    struct {
        PAL_HDR __in;
    } thread;

    struct {
        PAL_HDR __in;
    } semaphore;

    struct {
        PAL_HDR __in;
    } event;
} * PAL_HANDLE;

#endif /* PAL_HOST_H */
