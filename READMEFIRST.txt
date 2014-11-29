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

   
README: 
-------
Course Project: Porting PAL of Graphene to FreeBSD

Team: 
-------
Rajendra Kumar Raghupatruni
Thomas Mathew

Description:
--------------
This project is to port PAL component of Graphene to FreeBSD. 
Graphene is a Library OS developed at Stony Brook University under the guidance of Prof. Donald Porter. 
For more details about Graphene, see <https://github.com/oscarlab/graphene/wiki>

Status:
--------------
Most Pal/test binaries run successfully with the BSD code.(Except HandleSend)

Implemented Functionality:
----------------------------
- Resolved linking and relocation issues faced during compilation in BSD. The linker script used in the Makefile caused the Dynamic section(and the _DYNAMIC global variable) to go missing
- Modified syscall macros to match FreeBSD specifications. (syscall -> int 80h, minor tweaks to registers used etc.)
- Forced syscalls to return negative values on error (FreeBSD uses the carry flag). This allows the Linux error checking code to be reused.
- Replaced Linux-specific system calls with their BSD variants/alternates
- Modified system call flags/parameters according to BSD standards.
- Replaced Pal/lib/string implementations of memcpy, memmove, memcmp with native libc ones as the originals were not running correctly on BSD

Note:
- Currently, Linux compatibility is broken. We were forced to make changes in the common code to compile on BSD (missing header files etc.). We did not have time to update them with system-specific macros(ifdef)
- We have not compiled/tested LibOS, all our testing was done with Pal directly
- A lot of the advanced functionality of clone isn't present in BSD at the systemcall level. Pthreads would be the ideal option, but that involves linking libc-r

Acknowledgements:
-------------------
Many thanks to Chia-Che Tsai for helping us with our early linking issues and in solving "The case of the missing Dynamic section."

Pre-requisites:
-----------------
- Install FreeBSD 10.1 64-bit on a new machine or a VM
- For installation steps of FreeBSD, see <https://www.freebsd.org/doc/handbook/bsdinstall.html>
- Install GCC using the below command (if not already present)
	* pkg install gcc49
- Create a symbolic link for gcc (if not already present)
	* ln -s /usr/local/bin/gcc49 /usr/bin/gcc
- Install Gmake using the below command (if not already present)
	* pkg install gmake
- Create a symbolic link for gmake (if not already present)
	* ln -s /usr/local/bin/gmake /usr/bin/make

(We have modified the makefiles to use gmake and gcc49, but it's easier to just create the symlinks)

Download the source code from the git repository:
	* git clone http://github.com/susheeltm/graphene.git

Compilation Steps:
	* cd graphene/Pal/src
	* make or gmake
	* cd ../test
	* unzip test.zip

Note:
- test.zip file has the binaries compiled from Linux(BSD ELFs aren't supported).
Testing:
	* cd Pal/src
	* ./libpal.so ../test/HelloWorld
	* ./libpal.so ../test/Tcp(server)
	* ./libpal.so ../test/Tcp 1(client)
	* ./libpal.so ../test/Select

---------------------------------------------<<Thank You>>-----------------------------------------------------
