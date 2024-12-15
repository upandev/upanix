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
#include <MemManager.h>
#include <ElfSectionHeader.h>
#include <ElfConstants.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <BufferedReader.h>
#include <MountManager.h>
#include <GenericUtil.h>

using namespace ElfSectionHeader ;
using namespace ElfHeader ;
using namespace ElfRelocSection ;
using namespace ElfSymbolTable ;
using namespace ElfDynamicSection ;

# define REL_PLT_SUB_NAME	".rela.plt"

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

bool DynamicLinkLoader_GetSymbolOffsetFromProcess(ElfParser& elfParser, const char* szSymName, uint64_t* symAddress);

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

	Elf64_Ehdr* pELFHeader ;
	Elf64_Shdr* pELFSectionHeader ;
	char* pSecHeaderStrTable ;
	
	Elf64_Ehdr* pProcessELFHeader ;
	Elf64_Shdr* pProcessELFSectionHeader ;
	char* pProcessSecHeaderStrTable ;

	uint64_t uiBaseAddress ;

	if(iID >= 0) {
		DLLInfo& dllInfo = process.getDLLInfo(iID).value();
    uiBaseAddress = dllInfo.virtualLoadAddress();
    pELFHeader = (Elf64_Ehdr*) dllInfo.virtualLoadAddress();
    pELFSectionHeader = dllInfo.elfInfo().elfSectionHeaders();
    pSecHeaderStrTable = dllInfo.elfInfo().elfSecStrTable();
  } else {
		uiBaseAddress = 0;
		pELFHeader = (Elf64_Ehdr*)process.getProcessBase();
    pELFSectionHeader = process.getELFInfo().elfSectionHeaders();
    pSecHeaderStrTable = process.getELFInfo().elfSecStrTable();
  }

	ElfParser mELFParser(pELFHeader, pELFSectionHeader, pSecHeaderStrTable) ;

  Elf64_Shdr* pRelocationSectionHeader = mELFParser.GetSectionHeaderByTypeAndName(SHT_RELA, REL_PLT_SUB_NAME).goodValueOrThrow(XLOC);
  Elf64_Shdr* pDynamicSymSectionHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);
  __attribute__((unused)) Elf64_Shdr* pProcedureLinkSectionHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_info).goodValueOrThrow(XLOC);
  Elf64_Shdr* pDynamicSymStringSectionHeader = mELFParser.GetSectionHeaderByIndex(pDynamicSymSectionHeader->sh_link).goodValueOrThrow(XLOC);
	Elf64_Rela* pELFRelTable = (Elf64_Rela*)(GLOBAL_REL_ADDR(pRelocationSectionHeader->sh_addr, uiBaseAddress)) ;
	Elf64_Sym* pELFDynSymTable = (Elf64_Sym*)(GLOBAL_REL_ADDR(pDynamicSymSectionHeader->sh_addr, uiBaseAddress)) ;
	const char* pDynStrTable = (const char*)(GLOBAL_REL_ADDR(pDynamicSymStringSectionHeader->sh_addr, uiBaseAddress)) ;
	unsigned uiSymIndex = ELF64_R_SYM(pELFRelTable[relocationOffset].r_info);
	unsigned uiSymStrIndex = pELFDynSymTable[uiSymIndex].st_name ;
	char* szSymName = (char*)&pDynStrTable[uiSymStrIndex] ;
  //printf("\n %s", szSymName);
	pProcessELFHeader = (Elf64_Ehdr*)process.getProcessBase();
	pProcessELFSectionHeader = process.getELFInfo().elfSectionHeaders();
	pProcessSecHeaderStrTable = process.getELFInfo().elfSecStrTable();

	ElfParser mProgELFParser(pProcessELFHeader, pProcessELFSectionHeader, pProcessSecHeaderStrTable) ;

	if (iID >= 0) {
	  uint64_t uiDynSymAddress;
	  if (DynamicLinkLoader_GetSymbolOffsetFromProcess(mProgELFParser, szSymName, &uiDynSymAddress)) {
	    uiDynSymAddress += pELFRelTable[relocationOffset].r_addend;
	    uint64_t* uiGOTAddress = (uint64_t*)GLOBAL_REL_ADDR(pELFRelTable[relocationOffset].r_offset, uiBaseAddress);
	    uiGOTAddress[0] = uiDynSymAddress;
	    *dynamicSymAddress = uiDynSymAddress;
      return;
	  }
	}
  Elf64_Shdr* pDynamicSectionHeader = mProgELFParser.GetSectionHeaderByType(SHT_DYNAMIC).goodValueOrThrow(XLOC);
	
	unsigned uiIndex, uiNoOfEntries = pDynamicSectionHeader->sh_size / pDynamicSectionHeader->sh_entsize ;
	Elf64_Dyn* pELFDynSection = (Elf64_Dyn*)(GLOBAL_REL_ADDR(pDynamicSectionHeader->sh_addr, 0)) ;

  for(uiIndex = 0; uiIndex < uiNoOfEntries; uiIndex++)
	{
		if(pELFDynSection[uiIndex].d_tag == DT_NEEDED) // TODO: Maintain a Map of tagID and tagType
		{
      uint64_t uiDynSymOffset ;
			const char* pProcessDynStrTable ;
			if(iID >= 0)
			{
			  Elf64_Shdr* pProcRelocSectionHeader = mProgELFParser.GetSectionHeaderByTypeAndName(SHT_RELA, REL_PLT_SUB_NAME).goodValueOrThrow(XLOC);
			  Elf64_Shdr* pProcDynamicSymSecHeader = mProgELFParser.GetSectionHeaderByIndex(pProcRelocSectionHeader->sh_link).goodValueOrThrow(XLOC);
			  Elf64_Shdr* pProcDynamicSymStringSecHeader = mProgELFParser.GetSectionHeaderByIndex(pProcDynamicSymSecHeader->sh_link).goodValueOrThrow(XLOC);
			  pProcessDynStrTable = (const char*)(GLOBAL_REL_ADDR(pProcDynamicSymStringSecHeader->sh_addr, 0)) ;
			}
			else
			{
			  pProcessDynStrTable = pDynStrTable ;
			}
			char* szDLLName = (char*)&pProcessDynStrTable[ pELFDynSection[uiIndex].d_un.d_val ] ;

      if(DynamicLinkLoader_GetSymbolOffset(szDLLName, szSymName, &uiDynSymOffset, process)) {
        uint64_t* uiGOTAddress = (uint64_t*)GLOBAL_REL_ADDR(pELFRelTable[relocationOffset].r_offset, uiBaseAddress) ;
        uint64_t uiDynSymAddress = process.getDLLInfo(szDLLName).value().virtualLoadAddress() + uiDynSymOffset + pELFRelTable[relocationOffset].r_addend;
        uiGOTAddress[0] = uiDynSymAddress;
        *dynamicSymAddress = uiDynSymAddress;
        return;
      }
		}
	}

  throw upan::exception(XLOC, "Dynamic Symbol Look Up Failed: %s", szSymName);
}

bool DynamicLinkLoader_GetSymbolOffsetFromProcess(ElfParser& elfParser, const char* szSymName, uint64_t* symAddress) {
  const Elf64_Shdr* pDynSymTableSectionHeader = elfParser.GetSectionHeaderByType(SHT_DYNSYM).goodValueOrThrow(XLOC);
  const Elf64_Shdr* pDynSymStrTableSectionHeader = elfParser.GetSectionHeaderByIndex(pDynSymTableSectionHeader->sh_link).goodValueOrThrow(XLOC);
  const char* pSymStrTable = (const char*)(GLOBAL_REL_ADDR(pDynSymStrTableSectionHeader->sh_addr, 0)) ;

  const auto tableSize = pDynSymTableSectionHeader->sh_size / pDynSymTableSectionHeader->sh_entsize;
  const Elf64_Sym* entries = (Elf64_Sym*)(GLOBAL_REL_ADDR(pDynSymTableSectionHeader->sh_addr, 0)) ;
  for(uint32_t i = 0; i < tableSize; ++i) {
    if (strcmp(&pSymStrTable[entries[i].st_name], szSymName) == 0) {
      if (entries[i].st_value == 0) {
        break;
      }
      *symAddress = entries[i].st_value;
      return true;
    }
  }
  return false;
}

bool DynamicLinkLoader_GetSymbolOffset(const char* szJustDLLName, const char* szSymName, uint64_t* uiDynSymOffset, Process& process) {
  const auto& dllInfo = process.getDLLInfo(szJustDLLName).value();

  const auto pELFHeader = (Elf64_Ehdr*)(dllInfo.virtualLoadAddress());
  const auto pELFSectionHeader = dllInfo.elfInfo().elfSectionHeaders();
  upan::uniq_ptr<ElfParser> pELFParser(new ElfParser(pELFHeader, pELFSectionHeader, nullptr));

  const auto pHashSectionHeader = pELFParser->GetSectionHeaderByType(SHT_HASH).goodValueOrThrow(XLOC);
  const auto pDynamicSymSectionHeader = pELFParser->GetSectionHeaderByIndex(pHashSectionHeader->sh_link).goodValueOrThrow(XLOC);
  const auto pDynamicSymStringSectionHeader = pELFParser->GetSectionHeaderByIndex(pDynamicSymSectionHeader->sh_link).goodValueOrThrow(XLOC);
  const auto pELFDynSymTable = (Elf64_Sym*)(dllInfo.virtualLoadAddress() + pDynamicSymSectionHeader->sh_addr);
  const auto pDynStrTable = (const char*)(dllInfo.virtualLoadAddress() + pDynamicSymStringSectionHeader->sh_addr);

  auto pHashTable = (Elf64_Word*)(dllInfo.virtualLoadAddress() + pHashSectionHeader->sh_addr);
	auto uiNoOfBuckets = pHashTable[0];

	__attribute__((unused)) auto uiNoOfChains = pHashTable[1];
	auto pBucket = (Elf64_Word*)((Elf64_Word*)pHashTable + 2);
	auto pChain = (Elf64_Word*)((Elf64_Word*)pHashTable + 2 + uiNoOfBuckets);
	auto uiHashValue = DynamicLinkLoader_GetHashValue(szSymName);
	uint32_t uiSymTabIndex = pBucket[uiHashValue % uiNoOfBuckets];
  uint32_t uiSymStrIndex;

	for(; uiSymTabIndex != STN_UNDEF;) {
		uiSymStrIndex = pELFDynSymTable[uiSymTabIndex].st_name;
		if(strcmp(&pDynStrTable[uiSymStrIndex], szSymName) == 0) {
			if(pELFDynSymTable[uiSymTabIndex].st_value != 0) {
				*uiDynSymOffset = pELFDynSymTable[uiSymTabIndex].st_value;
        return true;
			}
		}
		uiSymTabIndex = pChain[uiSymTabIndex];
	}

  return false;
}

