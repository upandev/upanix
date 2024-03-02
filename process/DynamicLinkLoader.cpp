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
#include <DMM.h>
#include <ProcessLoader.h>
#include <MemUtil.h>
#include <BufferedReader.h>
#include <MountManager.h>
#include <GenericUtil.h>
#include <uniq_ptr.h>

using namespace ElfSectionHeader ;
using namespace ElfHeader ;
using namespace ElfRelocSection ;
using namespace ElfSymbolTable ;
using namespace ElfDynamicSection ;

# define REL_PLT_SUB_NAME	".plt"

/****************************** Static Functions *******************************************/
static unsigned DynamicLinkLoader_GetHashValue(const char* name)
{
    unsigned h = 0, g ;

    while(*name)
    {
        h = (h << 4) + *name++ ;

        if((g = (h & 0xf0000000)))
            h ^= g >> 24 ;

        h &= ~g ;
    }

    return h ;
}

static void DynamicLinkLoader_LoadDLL(const char* szJustDLLName, Process* processAddressSpace)
{
  auto dllInfo = processAddressSpace->getDLLInfo(szJustDLLName);
	if(dllInfo.isEmpty())
	{
		char szDLLFullName[128] ;
		char szLibPath[128] = "" ;

		if(!GenericUtil_GetFullFilePathFromEnv(LD_LIBRARY_PATH_ENV, LIB_PATH, szJustDLLName, szLibPath)) {
      throw upan::exception(XLOC, "DLL shared object file not found: %s", szJustDLLName);
    }

		strcpy(szDLLFullName, szLibPath) ;
		strcat(szDLLFullName, szJustDLLName) ;

		processAddressSpace->LoadELFDLL(szDLLFullName, szJustDLLName);
	}
}

static byte* DynamicLinkLoader_LoadDLLFileIntoMemory(const ElfParser& elfParser) {
	uint64_t minMemAddr, maxMemAddr ;
  elfParser.GetMemImageSize(minMemAddr, maxMemAddr) ;
	if(minMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC");

	uint64_t memImageSize = ProcessLoader_GetCeilAlignedAddress(maxMemAddr - minMemAddr, 4) ;

  upan::uniq_ptr<byte[]> dllImage(new byte[memImageSize]);

  elfParser.CopyProcessImage(dllImage.get(), 0, memImageSize);

  return dllImage.release();
}

bool DynamicLinkLoader_GetSymbolOffsetFromProcess(ElfParser& elfParser, const char* szSymName, unsigned* symAddress);

/**********************************************************************************************/

void DynamicLinkLoader_DoRelocation(Process* processAddressSpace, int iID, uint64_t relocationOffset, uint64_t *dynamicSymAddress) {
  //multithread synchronization
  upan::mutex_guard g(processAddressSpace->dllMutex().value());

	Elf64_Ehdr* pELFHeader ;
	Elf64_Shdr* pELFSectionHeader ;
	char* pSecHeaderStrTable ;
	
	Elf64_Ehdr* pProcessELFHeader ;
	Elf64_Shdr* pProcessELFSectionHeader ;
	char* pProcessSecHeaderStrTable ;

	unsigned uiBaseAddress ;

	if(iID >= 0) {
		ProcessDLLInfo& dllInfo = processAddressSpace->getDLLInfo(iID).value();
    uiBaseAddress = dllInfo.loadAddress();
    pELFHeader = (Elf64_Ehdr*) dllInfo.loadAddress();
    pELFSectionHeader = dllInfo.elfInfo()._elfSectionHeaders;
    pSecHeaderStrTable = dllInfo.elfInfo()._elfSecStrTable;
  } else {
		uiBaseAddress = 0;//PROCESS_BASE ;
		pELFHeader = (Elf64_Ehdr*)(GLOBAL_REL_ADDR(processAddressSpace->getProcessBase(), uiBaseAddress)) ;
    pELFSectionHeader = processAddressSpace->getELFInfo()._elfSectionHeaders;
    pSecHeaderStrTable = processAddressSpace->getELFInfo()._elfSecStrTable;

  }

	ElfParser mELFParser(pELFHeader, pELFSectionHeader, pSecHeaderStrTable) ;

  Elf64_Shdr* pRelocationSectionHeader = mELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_PLT_SUB_NAME).goodValueOrThrow(XLOC);
  Elf64_Shdr* pDynamicSymSectionHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);
  __attribute__((unused)) Elf64_Shdr* pProcedureLinkSectionHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_info).goodValueOrThrow(XLOC);
  Elf64_Shdr* pDynamicSymStringSectionHeader = mELFParser.GetSectionHeaderByIndex(pDynamicSymSectionHeader->sh_link).goodValueOrThrow(XLOC);

	Elf64_Rel* pELFRelTable = (Elf64_Rel*)(GLOBAL_REL_ADDR(pRelocationSectionHeader->sh_addr, uiBaseAddress)) ;

	Elf64_Sym* pELFDynSymTable = (Elf64_Sym*)(GLOBAL_REL_ADDR(pDynamicSymSectionHeader->sh_addr, uiBaseAddress)) ;
	const char* pDynStrTable = (const char*)(GLOBAL_REL_ADDR(pDynamicSymStringSectionHeader->sh_addr, uiBaseAddress)) ;

	unsigned uiSymIndex = ELF64_R_SYM(ELF64_REL_ENT(pELFRelTable, relocationOffset)->r_info) ;
	unsigned uiSymStrIndex = pELFDynSymTable[uiSymIndex].st_name ;
	char* szSymName = (char*)&pDynStrTable[uiSymStrIndex] ;

	pProcessELFHeader = (Elf64_Ehdr*)(GLOBAL_REL_ADDR(processAddressSpace->getProcessBase(), 0)) ;
	pProcessELFSectionHeader = processAddressSpace->getELFInfo()._elfSectionHeaders;
	pProcessSecHeaderStrTable = processAddressSpace->getELFInfo()._elfSecStrTable;

	ElfParser mProgELFParser(pProcessELFHeader, pProcessELFSectionHeader, pProcessSecHeaderStrTable) ;

	if (iID >= 0) {
	  uint32_t uiDynSymAddress;
	  if (DynamicLinkLoader_GetSymbolOffsetFromProcess(mProgELFParser, szSymName, &uiDynSymAddress)) {
	    unsigned* uiGOTAddress = (unsigned*)GLOBAL_REL_ADDR(ELF64_REL_ENT(pELFRelTable, relocationOffset)->r_offset, uiBaseAddress);
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
			unsigned uiDynSymOffset ;
			const char* pProcessDynStrTable ;
			if(iID >= 0)
			{
			  Elf64_Shdr* pProcRelocSectionHeader = mProgELFParser.GetSectionHeaderByTypeAndName(SHT_REL, REL_PLT_SUB_NAME).goodValueOrThrow(XLOC);
			  Elf64_Shdr* pProcDynamicSymSecHeader = mProgELFParser.GetSectionHeaderByIndex(pProcRelocSectionHeader->sh_link).goodValueOrThrow(XLOC);
			  Elf64_Shdr* pProcDynamicSymStringSecHeader = mProgELFParser.GetSectionHeaderByIndex(pProcDynamicSymSecHeader->sh_link).goodValueOrThrow(XLOC);
			  pProcessDynStrTable = (const char*)(GLOBAL_REL_ADDR(pProcDynamicSymStringSecHeader->sh_addr, 0)) ;
			}
			else
			{
			  pProcessDynStrTable = pDynStrTable ;
			}
			char* szDLLName = (char*)&pProcessDynStrTable[ pELFDynSection[uiIndex].d_un.d_val ] ;

      if(DynamicLinkLoader_GetSymbolOffset(szDLLName, szSymName, &uiDynSymOffset, processAddressSpace))
      {
        DynamicLinkLoader_LoadDLL(szDLLName, processAddressSpace);

        unsigned* uiGOTAddress = (unsigned*)GLOBAL_REL_ADDR(ELF64_REL_ENT(pELFRelTable, relocationOffset)->r_offset, uiBaseAddress) ;
        unsigned uiDynSymAddress = processAddressSpace->getDLLInfo(szDLLName).value().loadAddressForProcess() + uiDynSymOffset;
        uiGOTAddress[0] = uiDynSymAddress ;
        *dynamicSymAddress = uiDynSymAddress ;
        return;
      }
		}
	}

  throw upan::exception(XLOC, "Dynamic Symbol Look Up Failed: %s", szSymName);
}

bool DynamicLinkLoader_GetSymbolOffsetFromProcess(ElfParser& elfParser, const char* szSymName, unsigned* symAddress) {
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

bool DynamicLinkLoader_GetSymbolOffset(const char* szJustDLLName, const char* szSymName, unsigned* uiDynSymOffset, Process* processAddressSpace) {
  upan::uniq_ptr<byte[]> dllImage(nullptr);
  upan::uniq_ptr<ElfParser> pELFParser(nullptr);
  auto dllInfo = processAddressSpace->getDLLInfo(szJustDLLName);

	if(dllInfo.isEmpty())
	{
		char szDLLFullName[128] ;
		char szLibPath[128] = "" ;

		if(!GenericUtil_GetFullFilePathFromEnv(LD_LIBRARY_PATH_ENV, LIB_PATH, szJustDLLName, szLibPath))
      throw upan::exception(XLOC, "Failed to find dll (shared object): %s", szJustDLLName);

		strcpy(szDLLFullName, szLibPath) ;
		strcat(szDLLFullName, szJustDLLName) ;

    pELFParser.reset(new ElfParser(szDLLFullName));

    dllImage.reset(DynamicLinkLoader_LoadDLLFileIntoMemory(*pELFParser));
	}
	else
	{
    dllImage.disown();
    dllImage.reset((byte*) dllInfo.value().loadAddress());

    Elf64_Ehdr* pELFHeader = (Elf64_Ehdr*)(dllImage.get()) ;
    Elf64_Shdr* pELFSectionHeader = dllInfo.value().elfInfo()._elfSectionHeaders;
    pELFParser.reset(new ElfParser(pELFHeader, pELFSectionHeader, NULL));
	}

  Elf64_Shdr* pHashSectionHeader = pELFParser->GetSectionHeaderByType(SHT_HASH).goodValueOrThrow(XLOC);
  Elf64_Shdr* pDynamicSymSectionHeader = pELFParser->GetSectionHeaderByIndex(pHashSectionHeader->sh_link).goodValueOrThrow(XLOC);
  Elf64_Shdr* pDynamicSymStringSectionHeader = pELFParser->GetSectionHeaderByIndex(pDynamicSymSectionHeader->sh_link).goodValueOrThrow(XLOC);

  Elf64_Sym* pELFDynSymTable = (Elf64_Sym*)(dllImage.get() + pDynamicSymSectionHeader->sh_addr) ;
  const char* pDynStrTable = (const char*)(dllImage.get() + pDynamicSymStringSectionHeader->sh_addr) ;

  unsigned* pHashTable = (unsigned*)(dllImage.get() + pHashSectionHeader->sh_addr) ;
	unsigned uiNoOfBuckets = pHashTable[0] ;
	__attribute__((unused)) unsigned uiNoOfChains = pHashTable[1] ;
	unsigned* pBucket = (unsigned*)((unsigned*)pHashTable + 2) ;
	unsigned* pChain = (unsigned*)((unsigned*)pHashTable + 2 + uiNoOfBuckets) ;
	
	unsigned uiHashValue = DynamicLinkLoader_GetHashValue(szSymName) ;
	unsigned uiSymTabIndex = pBucket[uiHashValue % uiNoOfBuckets] ;
	unsigned uiSymStrIndex ;

	for(; uiSymTabIndex != STN_UNDEF;)
	{
		uiSymStrIndex = pELFDynSymTable[uiSymTabIndex].st_name ;
		if(strcmp(&pDynStrTable[uiSymStrIndex], szSymName) == 0)
		{
			if(pELFDynSymTable[uiSymTabIndex].st_value != 0)
			{
				*uiDynSymOffset = pELFDynSymTable[uiSymTabIndex].st_value ;
        return true;
			}
		}

		uiSymTabIndex = pChain[uiSymTabIndex] ;
	}

  return false;
}

