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

#include "ElfConstants.h"

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) (i & 0xFFFFFFFFL)
#define ELF64_R_INFO(s, t) (((s) << 32) + ((t) & 0xFFFFFFFFL))
#define ELF64_REL_ENT(p, off) ((Elf64_Rel*)((byte*)p + off))

namespace ElfRelocSection {
	typedef struct {
		Elf64_Addr  r_offset;
		Elf64_Xword r_info;
	} PACKED Elf64_Rel;

	typedef struct {
		Elf64_Addr   r_offset;
		Elf64_Xword  r_info;
		Elf64_Sxword r_addend;
	} PACKED Elf64_Rela;

	static const unsigned R_386_NONE = 0;
	static const unsigned R_386_32 = 1;
	static const unsigned R_386_PC32 = 2;
	static const unsigned R_386_GOT32  = 3;
	static const unsigned R_386_PLT32 = 4;
	static const unsigned R_386_COPY = 5;
	static const unsigned R_386_GLOB_DAT = 6;
	static const unsigned R_386_JMP_SLOT = 7;
	static const unsigned R_386_RELATIVE = 8;
	static const unsigned R_386_GOTOFF = 9;
	static const unsigned R_386_GOTPC = 10;

	static const char RelocationType[11][40] = {
		"R_386_NONE",
		"R_386_32",
		"R_386_PC32",
		"R_386_GOT32 ",
		"R_386_PLT32",
		"R_386_COPY",
		"R_386_GLOB_DAT",
		"R_386_JMP_SLOT",
		"R_386_RELATIVE",
		"R_386_GOTOFF",
		"R_386_GOTPC",
	};
};
