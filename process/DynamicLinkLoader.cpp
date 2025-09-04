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
#include <ProcessManager.h>
#include <DynamicLinkLoader.h>
#include <ElfConstants.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <GenericUtil.h>

using namespace ElfSectionHeader ;
using namespace ElfRelocSection ;
using namespace ElfSymbolTable ;
using namespace ElfDynamicSection ;

# define LIBC "libc.so"

/****************************** Static Functions *******************************************/
static unsigned long DynamicLinkLoader_GetHashValue(const char* name) {
  unsigned long h = 0, g;
  while (*name) {
    h = (h << 4) + *name++;
    if ((g = (h & 0xf0000000)))
      h ^= g >> 24;
    h &= 0x0fffffff;
  }
  return h;
}

bool DynamicLinkLoader_GetSymbolOffsetFromProcess(Process& process, const char* szSymName, uint64_t& symGOTAddress, int64_t relocationAddend) {
   const auto elfDynSymTable = process.getELFInfo().getDynSymTable().valueOrThrow(XLOC, "no dynamic symbol table found");

   for (Elf64_Xword i = 0; i < process.getELFInfo().getDynSymTableSize(); ++i) {
    if (strcmp(process.getELFInfo().getDynSymName(elfDynSymTable[i].st_name), szSymName) == 0) {
      if (elfDynSymTable[i].st_value == 0) {
        break;
      }
      symGOTAddress = relocationAddend + elfDynSymTable[i].st_value;
      return true;
    }
  }

  return false;
}

bool DynamicLinkLoader_GetSymbolOffset(const upan::string& dllName, const upan::string& symName,
                                       uint64_t& symGOTAddress, int64_t relocationAddend, Process& process) {
  auto& dllInfo = process.getDLLInfo(dllName).value();

  auto dynSymTable = dllInfo.elfInfo().getDynSymTable().valueOrThrow(XLOC, "no dynamic symbol table found");
  auto hashTable = dllInfo.elfInfo().getHashTable().valueOrThrow(XLOC, "elf hash table not found");
  auto uiNoOfBuckets = hashTable[0];

  __attribute__((unused)) auto uiNoOfChains = hashTable[1];
  auto pBucket = (Elf64_Word*)((Elf64_Word*)hashTable + 2);
  auto pChain = (Elf64_Word*)((Elf64_Word*)hashTable + 2 + uiNoOfBuckets);
  auto hashValue = DynamicLinkLoader_GetHashValue(symName.c_str());
  uint32_t symTabIndex = pBucket[hashValue % uiNoOfBuckets];

  for(; symTabIndex != STN_UNDEF;) {
    if(strcmp(dllInfo.elfInfo().getDynSymName(dynSymTable[symTabIndex].st_name), symName.c_str()) == 0) {
      if(dynSymTable[symTabIndex].st_value != 0) {
        symGOTAddress = dllInfo.elfInfo().getBase() + relocationAddend + dynSymTable[symTabIndex].st_value;
        return true;
      }
    }
    symTabIndex = pChain[symTabIndex];
  }

  return false;
}

/**********************************************************************************************/

extern uint8_t _runtime_dll_resolver;
extern uint8_t _runtime_dll_resolver_end;

DynamicLinkLoader::DynamicLinkLoader() {
  _dll_resolver_size = &_runtime_dll_resolver_end - &_runtime_dll_resolver;
  _dll_resolver = new byte[_dll_resolver_size];
  memcpy(_dll_resolver, (void*)&_runtime_dll_resolver, _dll_resolver_size);
  printf("\n DLL resolved loaded (size: %d)", _dll_resolver_size);
}

void DynamicLinkLoader_DoRelocation(Process& process, int64_t iID, uint64_t relocationOffset, uint64_t *dynamicSymAddress) {
  //printf("\n %lld, %lu", iID, relocationOffset);
  //multithread synchronization
  upan::mutex_guard g(process.dllMutex().value());

  const ELFInfo* elfInfo;

	if(iID >= 0) {
    elfInfo = &process.getDLLInfo(iID).value().elfInfo();
  } else {
    elfInfo = &process.getELFInfo();
  }

  auto elfDynRelPltTable = elfInfo->getDynRelPltTable().valueOrThrow(XLOC, "no plt relocation table found");
  auto elfDynSymTable = elfInfo->getDynSymTable().valueOrThrow(XLOC, "not dynamic symbol table found");

	const auto symIndex = ELF64_R_SYM(elfDynRelPltTable[relocationOffset].r_info);
	const auto szSymName = elfInfo->getDynSymName(elfDynSymTable[symIndex].st_name);
  auto& symGOTAddress = *(uint64_t*)GLOBAL_REL_ADDR(elfDynRelPltTable[relocationOffset].r_offset, elfInfo->getBase());
  const auto relocationAddend = elfDynRelPltTable[relocationOffset].r_addend;

  //printf("\n %s", szSymName);
	if (iID >= 0) {
	  if (DynamicLinkLoader_GetSymbolOffsetFromProcess(process, szSymName, symGOTAddress, relocationAddend)) {
	    *dynamicSymAddress = symGOTAddress;
      return;
	  }
	}

  bool libcChecked = false;
  const auto procDynSection = process.getELFInfo().getDynSection().valueOrThrow(XLOC, "no dynamic section found");
  for(Elf64_Xword i = 0; i < process.getELFInfo().getDynSectionSize(); ++i) {
    // TODO: Maintain a Map of tagID and tagType
		if(procDynSection[i].d_tag == DT_NEEDED) {
			const auto szDLLName = process.getELFInfo().getDynSymName(procDynSection[i].d_un.d_val);
      libcChecked = strcmp(szDLLName, LIBC) == 0;
      if(DynamicLinkLoader_GetSymbolOffset(szDLLName, szSymName, symGOTAddress, relocationAddend, process)) {
        *dynamicSymAddress = symGOTAddress;
        return;
      }
		}
	}

  if (!libcChecked) {
    if(DynamicLinkLoader_GetSymbolOffset(LIBC, szSymName, symGOTAddress, relocationAddend, process)) {
      *dynamicSymAddress = symGOTAddress;
      return;
    }
  }

  throw upan::exception(XLOC, "Dynamic Symbol Look Up Failed: %s", szSymName);
}