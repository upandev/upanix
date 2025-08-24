#!/bin/bash
#	 Upanix - An x86 based Operating System
#	 Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
#
#  I am making my contributions/submissions to this project solely in
#  my personal capacity and am not conveying any rights to any
#  intellectual property of any third parties.
#																			 
#	 This program is free software: you can redistribute it and/or modify
#	 it under the terms of the GNU General Public License as published by
#	 the Free Software Foundation, either version 3 of the License, or
#	 (at your option) any later version.
#																			 
#	 This program is distributed in the hope that it will be useful,
#	 but WITHOUT ANY WARRANTY; without even the implied warranty of
#	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	 GNU General Public License for more details.
#																			 
#	 You should have received a copy of the GNU General Public License
#	 along with this program.  If not, see <http://www.gnu.org/licenses/

#set UPANIX_HOME to upanix checkout directory path
export INCLUDE="-I./ \
-I${UPANIX_HOME}/bin -I${UPANIX_HOME}/kernel \
-I${UPANIX_HOME}/process \
-I${UPANIX_HOME}/display \
\
-I${UPANIX_HOME}/kernelprocs/console \
-I${UPANIX_HOME}/kernelprocs/session \
-I${UPANIX_HOME}/kernelprocs \
\
-I${UPANIX_HOME}/drivers/floppy \
-I${UPANIX_HOME}/drivers/keyboard \
-I${UPANIX_HOME}/drivers/mouse \
-I${UPANIX_HOME}/drivers/bus \
-I${UPANIX_HOME}/drivers/ide \
-I${UPANIX_HOME}/drivers/ide/vendorspec \
-I${UPANIX_HOME}/drivers/usb/ \
-I${UPANIX_HOME}/drivers/usb/uhci \
-I${UPANIX_HOME}/drivers/usb/ehci \
-I${UPANIX_HOME}/drivers/usb/xhci \
-I${UPANIX_HOME}/drivers/usb/devices \
-I${UPANIX_HOME}/drivers/video \
-I${UPANIX_HOME}/drivers/net \
-I${UPANIX_HOME}/drivers/net/mac80211 \
-I${UPANIX_HOME}/drivers/net/ath9k \
-I${UPANIX_HOME}/drivers/ \
\
-I${UPANIX_HOME}/util \
-I${UPANIX_HOME}/memory \
-I${UPANIX_HOME}/filesystem \
-I${UPANIX_HOME}/users \
-I${UPANIX_HOME}/exeparser \
-I${UPANIX_HOME}/syscall \
\
-I${UPANIX_HOME}/libc/include \
-I${UPANIX_HOME}/libc/sysdeps/mos/common/ \
-I${UPANIX_HOME}/libc/sysdeps/mos/common/bits \
\
-I${UPANIX_HOME}/libm/include \
-I${UPANIX_HOME}/libm/include/bits \
\
-I${UPANIX_HOME}/libmcpp/infra \
-I${UPANIX_HOME}/libmcpp/ds \
-I${UPANIX_HOME}/libmcpp/mem \
-I${UPANIX_HOME}/libmcpp/cal \
\
-I${UPANIX_HOME}/libcxxrt"

export GLOBAL_HEADERS="$UPANIX_HOME/util/Global.h $UPANIX_HOME/util/AsmUtil.h"

export COMPILER=i686-elf-g++
export C_COMPILER=i686-elf-gcc

EXCEPTION_SUP="-fexceptions -frtti"
if [ "$#" -eq "1" ] && [ "$1" = "NE" ]
then	
	EXCEPTION_SUP="-fno-exceptions -fno-rtti"
fi

EH_FLAGS=""
if [ "$#" -eq "1" ] && [ "$1" = "EH" ]
then	
	EH_FLAGS=" -DIN_GCC -DIN_LIBGCC2 -Dinhibit_libc -fbuilding-libgcc -fno-stack-protector"
fi

export CPP_FLAGS=" -c -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc \
-std=c++11 -nostdinc++ -fno-threadsafe-statics -fpermissive ${EXCEPTION_SUP}"
export C_FLAGS=" -c -std=c11 -O0 -Wall -ffreestanding -nodefaultlibs -nostdlib -nostartfiles -nostdinc ${EH_FLAGS}"
