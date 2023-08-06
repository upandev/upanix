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

namespace ElfDynamicSection {
	typedef struct {
		Elf64_Sxword	d_tag;

		union {
			Elf64_Xword d_val;
			Elf64_Addr	d_ptr;
		} PACKED d_un;

	} PACKED Elf64_Dyn;

	static const unsigned ADDR_TYPE_IGN = 0;
	static const unsigned ADDR_TYPE_VAL = 1;
	static const unsigned ADDR_TYPE_PTR = 2;

	static const unsigned DT_NULL = 0;
	static const unsigned DT_NEEDED = 1;
	static const unsigned DT_PLTRELSZ = 2;
	static const unsigned DT_PLTGOT = 3;
	static const unsigned DT_HASH = 4;
	static const unsigned DT_STRTAB = 5;
	static const unsigned DT_SYMTAB = 6;
	static const unsigned DT_RELA = 7;
	static const unsigned DT_RELASZ = 8;
	static const unsigned DT_RELAENT = 9;
	static const unsigned DT_STRSZ = 10;
	static const unsigned DT_SYMENT = 11;
	static const unsigned DT_INIT = 12;
	static const unsigned DT_FINI = 13;
	static const unsigned DT_SONAME = 14;
	static const unsigned DT_RPATH = 15;
	static const unsigned DT_SYMBOLIC = 16;
	static const unsigned DT_REL = 17;
	static const unsigned DT_RELSZ = 18;
	static const unsigned DT_RELENT = 19;
	static const unsigned DT_PLTREL = 20;
	static const unsigned DT_DEBUG = 21;
	static const unsigned DT_TEXTREL = 22;
	static const unsigned DT_JMPREL = 23;
	static const unsigned DT_LOPROC = 0x70000000;
	static const unsigned DT_HIPROC = 0x7fffffff ;

	static const unsigned NO_OF_DT_TYPES = 24;
};