/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
#pragma once

#include <ElfConstants.h>

namespace ElfProgramHeader {
	typedef struct {
		Elf64_Word	p_type;
    Elf64_Word	p_flags;
		Elf64_Off	  p_offset;
		Elf64_Addr	p_vaddr;
		Elf64_Addr	p_paddr;
		Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;

    Elf64_Xword p_align;
	} PACKED Elf64_Phdr;

	static const unsigned PT_NULL = 0;
	static const unsigned PT_LOAD = 1;
	static const unsigned PT_DYNAMIC = 2;
	static const unsigned PT_INTERP = 3;
	static const unsigned PT_NOTE = 4;
	static const unsigned PT_SHLIB = 5;
	static const unsigned PT_PHDR = 6;
  static const unsigned PT_TLS = 7;
	static const unsigned PT_LOPROC = 0x70000000;
	static const unsigned PT_HIPROC = 0x7fffffff ;

	static const unsigned PF_R = 0x4 ;
	static const unsigned PF_W = 0x2;
	static const unsigned PF_X = 0x1;

	static const unsigned MAX_PROG_TYPES = 7;
	namespace {
		static const char ProgramHeaderType[MAX_PROG_TYPES][30] = {
				"Null Program Header",
				"Loadable Segment",
				"Dynamic Linking Info",
				"Interpreter Info",
				"Auxiliary Info",
				"SHLIB Unspecified",
				"Program Header Info"
		};
	};

	const char* GetProgHeaderType(unsigned uiPType);
};
