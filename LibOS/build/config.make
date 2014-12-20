# config.make.  Generated from config.make.in by configure.
# Don't edit this file.  Put configuration parameters in configparms instead.

version = 2.17
release = stable

# Installation prefixes.
install_root = $(DESTDIR)
prefix = /usr/local/graphene
exec_prefix = ${prefix}
datadir = ${datarootdir}
libdir = ${exec_prefix}/lib
slibdir = 
localedir = 
sysconfdir = ${prefix}/etc
libexecdir = ${exec_prefix}/libexec
rootsbindir = 
infodir = ${datarootdir}/info
includedir = ${prefix}/include
datarootdir = ${prefix}/share
localstatedir = ${prefix}/var

# Should we use and build ldconfig?
use-ldconfig = yes

# Maybe the `ldd' script must be rewritten.
ldd-rewrite-script = sysdeps/unix/sysv/linux/x86_64/ldd-rewrite.sed

# System configuration.
config-machine = x86_64
base-machine = x86_64
config-vendor = unknown
config-os = linux-gnu
config-sysdirs =  sysdeps/unix/sysv/linux/x86_64/64/nptl sysdeps/unix/sysv/linux/x86_64/64 nptl/sysdeps/unix/sysv/linux/x86_64 nptl/sysdeps/unix/sysv/linux/x86 sysdeps/unix/sysv/linux/x86 sysdeps/unix/sysv/linux/x86_64 sysdeps/unix/sysv/linux/wordsize-64 nptl/sysdeps/unix/sysv/linux nptl/sysdeps/pthread sysdeps/pthread sysdeps/unix/sysv/linux sysdeps/gnu sysdeps/unix/inet nptl/sysdeps/unix/sysv sysdeps/unix/sysv sysdeps/unix/x86_64 nptl/sysdeps/unix sysdeps/unix sysdeps/posix nptl/sysdeps/x86_64/64 sysdeps/x86_64/64 sysdeps/x86_64/fpu/multiarch sysdeps/x86_64/fpu sysdeps/x86/fpu sysdeps/x86_64/multiarch nptl/sysdeps/x86_64 sysdeps/x86_64 sysdeps/x86 sysdeps/ieee754/ldbl-96 sysdeps/ieee754/dbl-64/wordsize-64 sysdeps/ieee754/dbl-64 sysdeps/ieee754/flt-32 sysdeps/wordsize-64 sysdeps/ieee754 sysdeps/generic
cflags-cpu = 
asflags-cpu = 

config-extra-cflags = 
config-cflags-sse4 = yes
config-cflags-avx = yes
config-cflags-sse2avx = yes
config-cflags-novzeroupper = yes
config-cflags-nofma = -ffp-contract=off
config-asflags-i686 = 

defines = 
sysheaders = 
sysincludes = 
c++-sysincludes = 
all-warnings = 

have-z-combreloc = yes
have-z-execstack = yes
have-Bgroup = yes
with-fp = yes
old-glibc-headers = no
unwind-find-fde = no
have-forced-unwind = yes
have-fpie = yes
have-mfma4 = yes
have-as-vis3 = 
gnu89-inline-CFLAGS = -fgnu89-inline
have-ssp = yes
have-selinux = no
have-libaudit = 
have-libcap = 
have-cc-with-libunwind = no
fno-unit-at-a-time = -fno-toplevel-reorder -fno-section-anchors
bind-now = no
have-hash-style = yes
use-default-link = no
output-format = elf64-x86-64

static-libgcc = -static-libgcc

versioning = yes
oldest-abi = default
exceptions = -fexceptions
multi-arch = default

mach-interface-list = 

have-bash2 = yes
have-ksh = yes

sizeof-long-double = 16

nss-crypt = no

# Configuration options.
build-shared = yes
build-pic-default= no
build-profile = no
build-bounded = no
build-static-nss = no
add-ons = nptl
add-on-subdirs = 
sysdeps-add-ons =  nptl
cross-compiling = no
force-install = yes
link-obsolete-rpc = no
build-nscd = no
use-nscd = no

# Build tools.
CC = gcc
CXX = g++
BUILD_CC = 
CFLAGS =  -O2 -U_FORTIFY_SOURCE -fno-stack-protector
CPPFLAGS-config = 
CPPUNDEFS = 
ASFLAGS-config =  -Wa,--noexecstack
AR = ar
NM = nm
MAKEINFO = makeinfo
AS = $(CC) -c
MIG = 
BISON = no
AUTOCONF = no
OBJDUMP = objdump
OBJCOPY = objcopy
READELF = readelf

# Installation tools.
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_INFO = /usr/sbin/install-info
LN_S = ln -s
MSGFMT = msgfmt

# Script execution tools.
BASH = /bin/bash
KSH = /bin/bash
AWK = gawk
PERL = /usr/bin/perl

# Additional libraries.
LIBGD = no

# Package versions and bug reporting configuration.
PKGVERSION = (GNU libc) 
PKGVERSION_TEXI = (GNU libc) 
REPORT_BUGS_TO = <http://www.gnu.org/software/libc/bugs.html>
REPORT_BUGS_TEXI = @uref{http://www.gnu.org/software/libc/bugs.html}

# More variables may be inserted below by configure.

override stddef.h = # The installed <stddef.h> seems to be libc-friendly.
