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

Implemented Functionality:
----------------------------
- Resolved relocations issues faced while porting to BSD
- Updated the syscall functionality(macros) to cater to FreeBSD

Note:
- Currently the Linux compatibility is broken.
- All the binaries used are compiled in Linux.


The below sections describes how to set-up and test the course project for CSE506.

Acknowledgements:
-------------------
Many thanks to Chia-Che Tsai for assisting us to resolve some critical dynamic relocation issues.

Pre-requisites:
-----------------
- Install a latest FreeBSD OS on a new machine or a VM
- For installation steps of FreeBSD, see <https://www.freebsd.org/doc/handbook/bsdinstall.html>
- Install GCC using the below command (if not already present)
	* pkg install gcc49
- Create a symbolic link for gcc (if not already present)
	* ln -s /usr/local/bin/gcc49 /usr/bin/gcc
- Install Gmake using the below command (if not already present)
	* pkg install gmake
- Create a symbolic link for gmake (if not already present)
	* ln -s /usr/local/bin/gmake /usr/bin/make

Download the source code from git repository using the below command on a FreeBSD Machine:
	* git clone http://github.com/susheeltm/graphene.git

Compilation Steps:
	* cd graphene/Pal/src
	* make or gmake
	* cd ../test
	* unzip test.zip
	* rm manifest

Note:
- test.zip file has the binaries compiled in Linux.

Testing:
	* cd Pal/src
	* ./libpal.so ../test/HelloWorld
	* ./libpal.so ../test/Tcp
	* ./libpal.so ../test/Select
	* ./libpal.so ../test/<Other Functions>

---------------------------------------------<<Thank You>>-----------------------------------------------------
