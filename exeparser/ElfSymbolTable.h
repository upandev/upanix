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

#define ELF64_ST_BIND(st_info) ( (st_info) >> 4 )
#define ELF64_ST_TYPE(st_info) ( (st_info) & 0xf )
#define ELF64_ST_INFO(bind, type) ( ((bind) << 4) + ((type) & 0xf) )

namespace ElfSymbolTable {
	typedef	struct	{
		Elf64_Word		st_name;
    unsigned char	st_info;
    unsigned char	st_other;
    Elf64_Half		st_shndx;
    Elf64_Addr		st_value;
		Elf64_Xword		st_size;
	} PACKED Elf64_Sym;

	typedef struct {
		unsigned table_size;
		Elf64_Sym* symTabEntries;
	} PACKED ElfSymTables;

	/**** Symbol Binding Types *******/
	static const unsigned STB_LOCAL = 0;
	static const unsigned STB_GLOBAL = 1;
	static const unsigned STB_WEAK = 2;
	static const unsigned STB_LOPROC = 13;
	static const unsigned STB_HIPROC = 15;

	/***** Symbol Types *******/
	static const unsigned STT_NOTYPE = 0;
	static const unsigned STT_OBJECT = 1;
	static const unsigned STT_FUNC = 2;
	static const unsigned STT_SECTION = 3;
	static const unsigned STT_FILE = 4;
	static const unsigned STT_LOPROC = 13;
	static const unsigned STT_HIPROC = 15;

	static const unsigned STN_UNDEF = 0;

};
