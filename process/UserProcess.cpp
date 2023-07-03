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

#include <uniq_ptr.h>
#include <try.h>
#include <UserProcess.h>
#include <UserThread.h>
#include <ProcessManager.h>
#include <ProcessLoader.h>
#include <DynamicLinkLoader.h>
#include <MountManager.h>
#include <UserManager.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <ElfDynamicSection.h>
#include <IODescriptorTable.h>
#include <ProcessEnv.h>
#include <ProcessGroup.h>
#include <DMM.h>
#include <GraphicsVideo.h>

#define REL_DYN_SUB_NAME	".dyn"
#define BSS_SEC_NAME		".bss"
#define DLL_ELF_SEC_HEADER_PAGE 1

UserProcess::UserProcess(const upan::string &name, int parentID, int userID,
                         bool isFGProcess, int noOfParams, char** args)
    : AutonomousProcess(name, parentID, isFGProcess), _iodTable(_processID, parentID) {
  _mainThreadID = _processID;
  _uiAUTAddress = NULL;

  Load(noOfParams, args);

  _noOfPagesForDLLPTE = 0;
  _totalNoOfPagesForDLL = 0;
  _processLDT.BuildForUser();

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
  _userID = userID == DERIVE_FROM_PARENT && !parentProcess.isEmpty() ? parentProcess.value().userID() : _userID;
}

UserThread& UserProcess::CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) {
  return *new UserThread(*this, threadCaller, entryAddress, arg);
}

void UserProcess::Load(int iNumberOfParameters, char** szArgumentList)
{
  ELFParser mELFParser(_name.c_str());

  unsigned uiMinMemAddr, uiMaxMemAddr;

  mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr);
  if(uiMinMemAddr < PROCESS_BASE)
    throw upan::exception(XLOC, "process min load address %x is less than PROCESS_BASE %x", uiMinMemAddr, PROCESS_BASE);

  if((uiMinMemAddr % PAGE_SIZE) != 0)
    throw upan::exception(XLOC, "process min load address %x is not page aligned", uiMinMemAddr);

  unsigned uiDLLSectionSize ;

  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

  unsigned uiProcessImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
  unsigned uiMemImageSize = uiProcessImageSize + uiDLLSectionSize ;

  _noOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize);

  if(_noOfPagesForProcess > MAX_PAGES_PER_PROCESS)
    throw upan::exception(XLOC, "process requires %u pages that's larger than supported %u", _noOfPagesForProcess, MAX_PAGES_PER_PROCESS);

  _processBase = uiMinMemAddr ;
  unsigned uiPageOverlapForProcessBase = ((_processBase / PAGE_SIZE) % PAGE_TABLE_ENTRIES) ;
  _noOfPagesForPTE = MemManager::Instance().GetPTESizeInPages(_noOfPagesForProcess + uiPageOverlapForProcessBase) + PROCESS_SPACE_FOR_OS;

  const uint32_t uiPDEAddress = AllocateAddressSpace();
  unsigned pRealELFSectionHeadeAddr = DynamicLinkLoader_Initialize(uiPDEAddress);
  unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeadeAddr) ;
  mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeadeAddr + uiCopySize)) ;

  upan::uniq_ptr<byte[]> bProcessImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] { mELFParser.CopyProcessImage(bProcessImage.get(), _processBase, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    DeAllocateAddressSpace();
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bProcessImage.get() + uiProcessImageSize), (void*)bDLLSectionImage.get(), uiDLLSectionSize) ;

  // Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bProcessImage.get(), uiMinMemAddr).onGood([&](uint32_t* uiGOT) {
    uiGOT[1] = -1;
    uiGOT[2] = uiMinMemAddr + uiProcessImageSize;
  });

  // Initialize BSS segment to 0
  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_NOBITS, BSS_SEC_NAME).onGood([&] (ELF32SectionHeader* pBSSSectionHeader) {
    unsigned *pBSS = (unsigned *) (bProcessImage.get() + pBSSSectionHeader->sh_addr - uiMinMemAddr);
    unsigned uiBSSSize = pBSSSectionHeader->sh_size;

    unsigned q;
    for (q = 0; q < uiBSSSize / sizeof(unsigned); q++) pBSS[q] = 0;

    unsigned r;
    for (r = q * sizeof(unsigned); r < uiBSSSize; r++) ((char *) pBSS)[r] = 0;
  });

  CopyElfImage(uiPDEAddress, bProcessImage.get(), uiMemImageSize);

  /* Find init and term stdio functions in libc if any */
  /*** This code is not required anymore because init and term is now handled in crt0.s - which calls init_standard_library and exit functions */
//  unsigned uiInitRelocAddress = NULL ;
//  unsigned uiTermRelocAddress = NULL ;
//  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_REL, REL_PLT_SUB_NAME).onGood([&] (ELF32SectionHeader* pRelocationSectionHeader)
//  {
//    mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).onGood([&] (ELF32SectionHeader* pDynamicSymSectiomHeader)
//    {
//      mELFParser.GetSectionHeaderByIndex(pDynamicSymSectiomHeader->sh_link).onGood([&] (ELF32SectionHeader* pDynamicSymStringSectionHeader)
//      {
//        ELFRelocSection::ELF32_Rel* pELFDynRelTable =
//          (ELFRelocSection::ELF32_Rel*)((unsigned)bProcessImage.get() + pRelocationSectionHeader->sh_addr - uiMinMemAddr) ;
//
//        unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;
//
//        ELFSymbolTable::ELF32SymbolEntry* pELFDynSymTable =
//          (ELFSymbolTable::ELF32SymbolEntry*)((unsigned)bProcessImage.get() + pDynamicSymSectiomHeader->sh_addr - uiMinMemAddr) ;
//
//        const char* pDynStrTable = (const char*)((unsigned)bProcessImage.get() + pDynamicSymStringSectionHeader->sh_addr - uiMinMemAddr) ;
//
//        for(unsigned i = 0; i < uiNoOfDynRelEntries && (uiInitRelocAddress == NULL || uiTermRelocAddress == NULL); i++)
//        {
//          if(ELF32_R_TYPE(pELFDynRelTable[i].r_info) == ELFRelocSection::R_386_JMP_SLOT)
//          {
//            int iSymIndex = ELF32_R_SYM(pELFDynRelTable[i].r_info);
//            int iStrIndex = pELFDynSymTable[ iSymIndex ].st_name ;
//            char* szSymName = (char*)&pDynStrTable[ iStrIndex ] ;
//
//            if(strcmp(szSymName, INIT_NAME) == 0)
//              uiInitRelocAddress = pELFDynRelTable[i].r_offset ;
//            else if(strcmp(szSymName, TERM_NAME) == 0)
//              uiTermRelocAddress = pELFDynRelTable[i].r_offset ;
//          }
//        }
//      });
//    });
//  });

  /* End of Find init and term stdio functions in libc if any */

  const uint32_t uiProcessEntryStackSize = PushProgramInitStackData(iNumberOfParameters, szArgumentList) ;
  const uint32_t uiEntryAdddress = mELFParser.GetProgramStartAddress();// uiMinMemAddr + uiProcessImageSize ;

  ProcessEnv_Initialize(uiPDEAddress, _parentProcessID);

  const uint32_t stackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_BASE;
  _taskState.BuildForUser(stackTopAddress, uiPDEAddress, uiEntryAdddress, uiProcessEntryStackSize);
}

uint32_t UserProcess::PushProgramInitStackData(int iNumberOfParameters, char** szArgumentList) {
  const uint32_t uiPTEIndex = PAGE_TABLE_ENTRIES - PROCESS_CG_STACK_PAGES - NO_OF_PAGES_FOR_STARTUP_ARGS;
  uint32_t uiPageAddress = (((unsigned*)(_stackPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex]) & 0xFFFFF000 ;

  const unsigned argc = 4;
  const unsigned argv = 4; // address of argv first dimension
  const unsigned uiArgumentAddressListSize = iNumberOfParameters * 4; // address of char* entry (second dimension) of argv array

  unsigned uiStackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_CG_STACK_PAGES * PAGE_SIZE - PROCESS_BASE;

  unsigned uiArgumentDataSize = 0;

  for(int i = 0; i < iNumberOfParameters; i++)
    uiArgumentDataSize += (strlen(szArgumentList[i]) + 1) ;

  const uint32_t uiProcessEntryStackSize = argc + argv + uiArgumentAddressListSize + uiArgumentDataSize;

  if (uiProcessEntryStackSize > (NO_OF_PAGES_FOR_STARTUP_ARGS * PAGE_SIZE)) {
    throw upan::exception(XLOC, "Startup arguments size is larger than %d pages", NO_OF_PAGES_FOR_STARTUP_ARGS);
  }

  uiPageAddress = uiPageAddress + PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE - uiProcessEntryStackSize ;

  int iStackIndex = 0 ;
  ((unsigned*)(uiPageAddress))[iStackIndex++] = iNumberOfParameters; // argc
  int argLength = (iStackIndex + 1) * 4;
  ((unsigned*)(uiPageAddress))[iStackIndex++] = uiStackTopAddress - uiProcessEntryStackSize + argLength; // argv

  uiArgumentDataSize = 0 ;// argv[0] through argv[argc - 1]
  for(int i = 0; i < iNumberOfParameters; i++) {
    ((unsigned*)(uiPageAddress))[iStackIndex + i] = ((unsigned*)(uiPageAddress))[iStackIndex - 1] + uiArgumentAddressListSize + uiArgumentDataSize ;
    strcpy((char*)&((unsigned*)(uiPageAddress))[iStackIndex + iNumberOfParameters] + uiArgumentDataSize,	szArgumentList[i]) ;
    uiArgumentDataSize += (strlen(szArgumentList[i]) + 1) ;
  }

  return uiProcessEntryStackSize;
}

void UserProcess::CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize)
{
  unsigned uiPDEAddress = uiPDEAddr ;
  unsigned uiPDEIndex, uiPTEIndex, uiPTEAddress, uiPageAddress ;
  unsigned uiOffset = 0 ;
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
  unsigned uiProcessPageBase = ( _processBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

  unsigned uiCopySize = uiMemImageSize ;
  unsigned uiNoOfPagesForProcess = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) ;

  for(uint32_t i = 0; i < uiNoOfPagesForProcess; i++)
  {
    uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
    uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
    //Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall
    //be aligned at PAGE BOUNDARY

    uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
    uiPageAddress = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex]) & 0xFFFFF000 ;

    memcpy((void*)(uiPageAddress - GLOBAL_DATA_SEGMENT_BASE), (void*)((unsigned)bProcessImage + uiOffset), upan::min(uiCopySize, (uint32_t)PAGE_SIZE));
    if(uiCopySize <= PAGE_SIZE)
      break;

    uiOffset += PAGE_SIZE ;
    uiCopySize -= PAGE_SIZE ;
  }

  return ;
}

void UserProcess::LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) {
  ELFParser mELFParser(szDLLName) ;

  unsigned uiMinMemAddr, uiMaxMemAddr ;

  mELFParser.GetMemImageSize(&uiMinMemAddr, &uiMaxMemAddr) ;
  if(uiMinMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC - DLL Min Address: %x", uiMinMemAddr);

  unsigned uiDLLSectionSize ;
  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

  const uint32_t uiDLLImageSize = ProcessLoader_GetCeilAlignedAddress(uiMaxMemAddr - uiMinMemAddr, 4) ;
  const uint32_t uiMemImageSize = uiDLLImageSize + uiDLLSectionSize ;
  const uint32_t uiNoOfPagesForDLL = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + DLL_ELF_SEC_HEADER_PAGE ;

  if(uiNoOfPagesForDLL > MAX_PAGES_PER_PROCESS)
    throw upan::exception(XLOC, "No. of pages for DLL %u exceeds max limit per dll %u", uiNoOfPagesForDLL, MAX_PAGES_PER_PROCESS);

  if(!KC::MKernelService().RequestDLLAlloCopy(uiNoOfPagesForDLL, szJustDLLName)) {
    throw upan::exception(XLOC, "Failed to allocate memory for DLL via kernal service");
  }

  const ProcessDLLInfo& dllInfo = getDLLInfo(szJustDLLName).value();
  const uint32_t uiDLLLoadAddress = dllInfo.loadAddressForProcess();
  const uint32_t pRealELFSectionHeaderAddr = dllInfo.elfSectionHeaderAddress();

  unsigned uiCopySize = mELFParser.CopyELFSectionHeader((ELF32SectionHeader*)pRealELFSectionHeaderAddr) ;
  mELFParser.CopyELFSecStrTable((char*)(pRealELFSectionHeaderAddr + uiCopySize)) ;

  upan::uniq_ptr<byte[]> bDLLImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] () { mELFParser.CopyProcessImage(bDLLImage.get(), 0, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bDLLImage.get() + uiDLLImageSize), (void*)bDLLSectionImage.get(), uiDLLSectionSize);

  // Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bDLLImage.get(), uiMinMemAddr).onGood([&](uint32_t* uiGOT) {
    uiGOT[1] = dllInfo.id();
    uiGOT[2] = uiDLLImageSize + uiDLLLoadAddress;

    mELFParser.GetNoOfGOTEntries().onGood([&](uint32_t uiNoOfGOTEntries) {
      for(uint32_t i = 3; i < uiNoOfGOTEntries; i++)
        uiGOT[i] += uiDLLLoadAddress ;
    });
  });

/* Dynamic Relocation Entries are resolved here in Global Offset Table */
  mELFParser.GetSectionHeaderByTypeAndName(ELFSectionHeader::SHT_REL, REL_DYN_SUB_NAME).onGood([&] (ELF32SectionHeader* pRelocationSectionHeader) {
    ELF32SectionHeader *pDynamicSymSectiomHeader = mELFParser.GetSectionHeaderByIndex(
        pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);

    ELFRelocSection::ELF32_Rel* pELFDynRelTable = (ELFRelocSection::ELF32_Rel*)((unsigned) bDLLImage.get() + pRelocationSectionHeader->sh_addr);
    unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize;

    ELFSymbolTable::ELF32SymbolEntry *pELFDynSymTable = (ELFSymbolTable::ELF32SymbolEntry*)((unsigned) bDLLImage.get() + pDynamicSymSectiomHeader->sh_addr);

    for (uint32_t i = 0; i < uiNoOfDynRelEntries; i++) {
      const auto uiRelType = ELF32_R_TYPE(pELFDynRelTable[i].r_info);

      if (uiRelType == ELFRelocSection::R_386_RELATIVE) {
        ((unsigned *) ((unsigned) bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] += uiDLLLoadAddress;
      } else if (uiRelType == ELFRelocSection::R_386_GLOB_DAT) {
        ((unsigned *) ((unsigned) bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] =
            pELFDynSymTable[ELF32_R_SYM(pELFDynRelTable[i].r_info)].st_value + uiDLLLoadAddress;
      }
    }
  });
  /* End of Dynamic Relocation Entries resolution */
  memcpy((void*)dllInfo.loadAddressForKernel(), bDLLImage.get(), uiMemImageSize);
}

uint32_t UserProcess::AllocateAddressSpace() {
  const uint32_t uiPDEAddress = SchedulableProcess::Common::AllocatePDE();
  AllocatePTE(uiPDEAddress);
  InitializeProcessSpaceForOS(uiPDEAddress);
  InitializeProcessSpaceForProcess(uiPDEAddress);
  return uiPDEAddress;
}

void UserProcess::AllocatePTE(const unsigned uiPDEAddress)
{
  unsigned uiPDEIndex = 0;
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForPTE; i++)
  {
    auto uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

    for(uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++)
      ((unsigned*)((uiFreePageNo * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2 ;

    if(i < PROCESS_SPACE_FOR_OS) {
      // Full Permission is given at the PDE level and access control is enforced at PTE level i.e, individual page level
      ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7;
    }
    else
    {
      uiPDEIndex = i + uiProcessPDEBase ;
      ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
    }
  }

  _startPDEForDLL = ++uiPDEIndex;
  _stackPTEAddress = SchedulableProcess::Common::AllocatePTEForStack();
}

void UserProcess::InitializeProcessSpaceForOS(const unsigned uiPDEAddress)
{
  for(uint32_t i = 0; i < PROCESS_SPACE_FOR_OS; ++i)
  {
    unsigned kernelPTEAddress = (((unsigned*)(MEM_PDBR - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000;
    unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000 ;
    for(uint32_t  j = 0; j < PAGE_TABLE_ENTRIES; ++j)
    {
      unsigned kernelPageMapValue = ((unsigned*)(kernelPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j];
      ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = (kernelPageMapValue & 0xFFFFF000) | 0x5;
    }
  }
}

void UserProcess::InitializeProcessSpaceForProcess(const unsigned uiPDEAddress)
{
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
  unsigned uiProcessPageBase = ( _processBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForProcess; i++)
  {
    uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

    auto uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
    auto uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
    //Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall be aligned at PAGE BOUNDARY

    auto uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7;
  }

  SchedulableProcess::Common::AllocateStackSpace(_stackPTEAddress);
}

void UserProcess::DeAllocateResources() {
  DeAllocateGUIFramebuffer();

  DeAllocateDLLPages() ;
  DynamicLinkLoader_UnInitialize(this) ;

  DMM_DeAllocatePhysicalPages(this) ;

  ProcessEnv_UnInitialize(*this) ;

  DeAllocateAddressSpace();
}

void UserProcess::DeAllocateDLLPages() {
  unsigned* uiPDEAddress = ((unsigned*)(_taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE));
  uint32_t noOfPagesForDLL = _totalNoOfPagesForDLL;

  for(uint32_t i = 0; i < _noOfPagesForDLLPTE; i++) {
    auto isPTEPagePresent = uiPDEAddress[i + _startPDEForDLL] & 0x1;
    auto uiPTEAddress = uiPDEAddress[i + _startPDEForDLL] & 0xFFFFF000;

    if(isPTEPagePresent && uiPTEAddress != 0) {
      for(uint32_t k = 0; k < PAGE_TABLE_ENTRIES && noOfPagesForDLL > 0; --noOfPagesForDLL, ++k) {
        auto pageAddress = ((unsigned*)uiPTEAddress)[k];
        auto isPagePresent = pageAddress & 0x1;
        pageAddress &= 0xFFFFF000;
        if (isPagePresent && pageAddress != 0) {
          MemManager::Instance().DeAllocatePhysicalPage(pageAddress / PAGE_SIZE);
        }
      }
      MemManager::Instance().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE);
    }
  }
}

void UserProcess::DeAllocateAddressSpace() {
  DeAllocateProcessSpace();
  DeAllocatePTE();
  MemManager::Instance().DeAllocatePhysicalPage(_taskState.CR3_PDBR / PAGE_SIZE);
}

void UserProcess::DeAllocateProcessSpace()
{
  unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
  unsigned uiProcessPageBase = ( _processBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForProcess; i++)
  {
    auto uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
    auto uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
    //Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall be aligned at PAGE BOUNDARY

    auto uiPTEAddress = (((unsigned*)(_taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

    MemManager::Instance().DeAllocatePhysicalPage(
        (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE) ;
  }

  SchedulableProcess::Common::DeAllocateStackSpace(_stackPTEAddress);
}

void UserProcess::DeAllocatePTE() {
  const unsigned uiProcessPDEBase = _processBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

  for(uint32_t i = 0; i < _noOfPagesForPTE; i++) {
    const uint32_t pdeIndex = (i < PROCESS_SPACE_FOR_OS) ? i : i + uiProcessPDEBase;
    const uint32_t uiPTEAddress = ((unsigned*)(_taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[pdeIndex] & 0xFFFFF000;
    MemManager::Instance().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE) ;
  }

  MemManager::Instance().DeAllocatePhysicalPage(_stackPTEAddress / PAGE_SIZE);
}

void UserProcess::MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) {
  const unsigned uiNoOfPagesForDLLPTE = MemManager::Instance().GetPTESizeInPages(_totalNoOfPagesForDLL + noOfPagesForDLL) ;
  const unsigned uiPDEAddress = _taskState.CR3_PDBR ;

  if(uiNoOfPagesForDLLPTE > _noOfPagesForDLLPTE) {
    for(uint32_t i = _noOfPagesForDLLPTE; i < uiNoOfPagesForDLLPTE; i++)
    {
      auto uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
      auto uiPDEIndex = _startPDEForDLL + i;
      ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
    }
  }

  for(uint32_t i = 0; i < noOfPagesForDLL; i++)
  {
    auto uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

    auto uiPDEIndex = (i + _totalNoOfPagesForDLL) / PAGE_TABLE_ENTRIES + _startPDEForDLL ;
    auto uiPTEIndex = (i + _totalNoOfPagesForDLL) % PAGE_TABLE_ENTRIES ;

    auto uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
  }

  auto loadAddress = _startPDEForDLL * PAGE_SIZE * PAGE_TABLE_ENTRIES + _totalNoOfPagesForDLL * PAGE_SIZE;
  _loadedDLLs.push_back(dllName);
  _dllInfoMap.insert(DLLInfoMap::value_type(dllName, ProcessDLLInfo(_loadedDLLs.size() - 1, loadAddress, noOfPagesForDLL)));

  _noOfPagesForDLLPTE = uiNoOfPagesForDLLPTE;
  _totalNoOfPagesForDLL += noOfPagesForDLL;
}

upan::option<const ProcessDLLInfo&> UserProcess::getDLLInfo(const upan::string& dllName) const {
  auto it = _dllInfoMap.find(dllName);
  if (it == _dllInfoMap.end()) {
    return upan::option<const ProcessDLLInfo&>::empty();
  }
  return upan::option<const ProcessDLLInfo&>(it->second);
}

upan::option<const ProcessDLLInfo&> UserProcess::getDLLInfo(int id) const {
  if (id < 0 || id >= _loadedDLLs.size()) {
    return upan::option<const ProcessDLLInfo&>::empty();
  }
  return getDLLInfo(_loadedDLLs[id]);
}

void UserProcess::onLoad() {
  SchedulableProcess::Common::UpdatePDEWithStackPTE(_taskState.CR3_PDBR, _stackPTEAddress);
}

void UserProcess::allocateGUIFramebuffer() {
  auto frameBufferAddress = GraphicsVideo::Instance().allocateFrameBuffer();
  const auto guiFramebufferPTEAddress = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  for (uint32_t i = 0; i < GraphicsVideo::Instance().LFBPageCount(); ++i) {
    ((unsigned *) (guiFramebufferPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = ((frameBufferAddress + (i * PAGE_SIZE)) & 0xFFFFF000) | 0x7;
  }
  ((unsigned *) (_taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_GUI_FRAMEBUFFER_PDE_ID] = (guiFramebufferPTEAddress & 0xFFFFF000) | 0x7;

  FrameBufferInfo frameBufferInfo;
  const auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  frameBufferInfo._pitch = f->_pitch;
  frameBufferInfo._width = f->_width;
  frameBufferInfo._height = f->_height;
  frameBufferInfo._bpp = f->_bpp;
  frameBufferInfo._frameBuffer = (uint32_t*)frameBufferAddress;
  upanui::FrameBuffer frameBuffer(frameBufferInfo);
  upanui::Viewport viewport(0, 0, frameBufferInfo._width, frameBufferInfo._height);
  auto rootFrame = new RootFrame(frameBuffer, viewport);
  rootFrame->enableDoubleBuffer(true);
  _frame.reset(rootFrame);

  //let processes write to stdout of the parent for debugging purpose.
  //_iodTable.setupNullStdOut();

  GraphicsVideo::Instance().addFGProcess(processID());
}

void UserProcess::initGuiFrame() {
  if (_frame.get() == nullptr) {
    upan::mutex_guard g(_addressSpaceMutex);
    KC::MKernelService().RequestProcessGUIFramebufferAllocate(*this);
  }
}

void UserProcess::DeAllocateGUIFramebuffer() {
  if (_frame.get() != nullptr) {
    DMM_DeAllocateForKernel((uint64_t)_frame->frameBuffer().buffer());
    const auto guiFramebufferPTEAddress = ((unsigned *) (_taskState.CR3_PDBR - GLOBAL_DATA_SEGMENT_BASE))[PROCESS_GUI_FRAMEBUFFER_PDE_ID] & 0xFFFFF000;
    MemManager::Instance().DeAllocatePhysicalPage(guiFramebufferPTEAddress / PAGE_SIZE);
    GraphicsVideo::Instance().removeFGProcess(processID());
  }
}