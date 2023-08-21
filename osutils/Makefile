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
CC = ${HOME}/opt/cross/bin/x86_64-elf-gcc
ASM = ${HOME}/opt/cross/bin/nasm

DLL_SRC = dll.asm 
DLL_OUTPUT = .dll

OUTPUTS := ${DLL_OUTPUT} crti.o crtn.o

osutils: ${OUTPUTS}

${DLL_OUTPUT}: ${DLL_SRC}
	${ASM} -fbin ${DLL_SRC} -o ${DLL_OUTPUT}

crti.o: crti.s
	${CC} -c crti.s

crtn.o: crtn.s
	${CC} -c crtn.s

clean:
	rm -f *.o
	rm -f ${OUTPUTS}

distclean:
	rm -f *.o
	rm -f ${OUTPUTS}

