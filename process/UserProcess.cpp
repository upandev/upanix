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
#include <IODescriptorTable.h>
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
  _autAddress = nullptr;
  _pml4Table = nullptr;

  Load(noOfParams, args);

  _totalNoOfPagesForDLL = 0;
  _processLDT.BuildForUser();

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
  _userID = userID == DERIVE_FROM_PARENT && !parentProcess.isEmpty() ? parentProcess.value().userID() : _userID;
}

UserThread& UserProcess::CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) {
  return *new UserThread(*this, threadCaller, entryAddress, arg);
}

void UserProcess::Load(int iNumberOfParameters, char** szArgumentList) {
  ElfParser mELFParser(_name.c_str());

  uint64_t minMemAddr, maxMemAddr;

  mELFParser.GetMemImageSize(minMemAddr, maxMemAddr);
  if(minMemAddr < PROCESS_BASE)
    throw upan::exception(XLOC, "process min load address %x is less than PROCESS_BASE %x", minMemAddr, PROCESS_BASE);

  if((minMemAddr % PAGE_SIZE) != 0)
    throw upan::exception(XLOC, "process min load address %x is not page aligned", minMemAddr);

  unsigned uiDLLSectionSize ;

  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

  uint64_t processImageSize = ProcessLoader_GetCeilAlignedAddress(maxMemAddr - minMemAddr, 4) ;
  uint64_t uiMemImageSize = processImageSize + uiDLLSectionSize ;

  _processBase = minMemAddr;
  uint64_t alignAdjustProcessSpace = _processBase % PAGE_SIZE ? PAGE_SIZE : 0;
  _processSpaceSize = uiMemImageSize + alignAdjustProcessSpace;

  if(_processSpaceSize > MAX_PROCESS_SPACE_SIZE) {
    throw upan::exception(XLOC, "process requires %lu space that's larger than supported %lu", _processSpaceSize, MAX_PROCESS_SPACE_SIZE);
  }

  AllocateAddressSpace();
  _elfInfo._elfSectionHeaders = mELFParser.CopyELFSectionHeader();
  _elfInfo._elfSecStrTable = mELFParser.CopyELFSecStrTable();

  upan::uniq_ptr<byte[]> bProcessImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] { mELFParser.CopyProcessImage(bProcessImage.get(), _processBase, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    DeallocateResources();
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bProcessImage.get() + processImageSize), (void*)bDLLSectionImage.get(), uiDLLSectionSize) ;

  // Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bProcessImage.get(), minMemAddr).onGood([&](uint64_t* uiGOT) {
    uiGOT[1] = -1;
    uiGOT[2] = minMemAddr + processImageSize;
  });

  // Initialize BSS segment to 0
  mELFParser.GetSectionHeaderByTypeAndName(ElfSectionHeader::SHT_NOBITS, BSS_SEC_NAME).onGood([&] (Elf64_Shdr* bssSectionHeader) {
    void* bss = (void*) (bProcessImage.get() + bssSectionHeader->sh_addr - minMemAddr);
    memset(bss, 0, bssSectionHeader->sh_size);
  });

  CopyElfImage(bProcessImage.get(), uiMemImageSize);

  /* Find init and term stdio functions in libc if any */
  /*** This code is not required anymore because init and term is now handled in crt0.s - which calls init_standard_library and exit functions */
//  unsigned uiInitRelocAddress = NULL ;
//  unsigned uiTermRelocAddress = NULL ;
//  mELFParser.GetSectionHeaderByTypeAndName(ElfSectionHeader::SHT_REL, REL_PLT_SUB_NAME).onGood([&] (Elf64_Shdr* pRelocationSectionHeader)
//  {
//    mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).onGood([&] (Elf64_Shdr* pDynamicSymSectiomHeader)
//    {
//      mELFParser.GetSectionHeaderByIndex(pDynamicSymSectiomHeader->sh_link).onGood([&] (Elf64_Shdr* pDynamicSymStringSectionHeader)
//      {
//        ElfRelocSection::Elf64_Rel* pELFDynRelTable =
//          (ElfRelocSection::Elf64_Rel*)((unsigned)bProcessImage.get() + pRelocationSectionHeader->sh_addr - minMemAddr) ;
//
//        unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize ;
//
//        ElfSymbolTable::Elf64_Sym* pELFDynSymTable =
//          (ElfSymbolTable::Elf64_Sym*)((unsigned)bProcessImage.get() + pDynamicSymSectiomHeader->sh_addr - minMemAddr) ;
//
//        const char* pDynStrTable = (const char*)((unsigned)bProcessImage.get() + pDynamicSymStringSectionHeader->sh_addr - minMemAddr) ;
//
//        for(unsigned i = 0; i < uiNoOfDynRelEntries && (uiInitRelocAddress == NULL || uiTermRelocAddress == NULL); i++)
//        {
//          if(ELF64_R_TYPE(pELFDynRelTable[i].r_info) == ElfRelocSection::R_386_JMP_SLOT)
//          {
//            int iSymIndex = ELF64_R_SYM(pELFDynRelTable[i].r_info);
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

  const uint32_t uiProcessEntryStackSize = PushProgramInitStackData(iNumberOfParameters, szArgumentList);
  const uint64_t uiEntryAdddress = mELFParser.GetProgramStartAddress();// minMemAddr + processImageSize ;

  const uint64_t stackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_BASE;
  _taskState.BuildForUser(stackTopAddress, (uint64_t)_pml4Table, uiEntryAdddress, uiProcessEntryStackSize);
}

uint32_t UserProcess::PushProgramInitStackData(int iNumberOfParameters, char **szArgumentList) {
  const unsigned argc = 4;
  const unsigned argv = 4; // address of argv first dimension
  const unsigned uiArgumentAddressListSize = iNumberOfParameters * 4; // address of char* entry (second dimension) of argv array

  const auto virtualStackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_CG_STACK_SIZE - PROCESS_BASE;

  uint32_t uiArgumentDataSize = 0;
  for(int i = 0; i < iNumberOfParameters; i++)
    uiArgumentDataSize += (strlen(szArgumentList[i]) + 1) ;

  const uint32_t processEntryStackSize = argc + argv + uiArgumentAddressListSize + uiArgumentDataSize;

  if (processEntryStackSize > PROCESS_INIT_STACK_SIZE) {
    throw upan::exception(XLOC, "Startup arguments size is larger than reserved init stack size of %u", PROCESS_INIT_STACK_SIZE);
  }

  const auto realStackTopAddress = MemManager::Instance().GetFlatAddressFromPD((uint64_t*)_stackPDAddress, virtualStackTopAddress) - processEntryStackSize;

  int iStackIndex = 0 ;
  ((unsigned*)(realStackTopAddress))[iStackIndex++] = iNumberOfParameters; // argc
  int argLength = (iStackIndex + 1) * 4;
  ((unsigned*)(realStackTopAddress))[iStackIndex++] = virtualStackTopAddress - processEntryStackSize + argLength; // argv

  uiArgumentDataSize = 0 ;// argv[0] through argv[argc - 1]
  for(int i = 0; i < iNumberOfParameters; i++) {
    ((unsigned*)(realStackTopAddress))[iStackIndex + i] = ((unsigned*)(realStackTopAddress))[iStackIndex - 1] + uiArgumentAddressListSize + uiArgumentDataSize ;
    strcpy((char*)&((unsigned*)(realStackTopAddress))[iStackIndex + iNumberOfParameters] + uiArgumentDataSize,	szArgumentList[i]) ;
    uiArgumentDataSize += (strlen(szArgumentList[i]) + 1) ;
  }

  return processEntryStackSize;
}

void UserProcess::CopyElfImage(byte* processImage, unsigned memImageSize) {
  uint64_t copySize = memImageSize;
  uint64_t offset = 0;

  uint64_t virtualAddress = _processBase + PROCESS_BASE;
  const uint64_t endAddress = virtualAddress + _processSpaceSize;

  for(; virtualAddress < endAddress; virtualAddress += PAGE_SIZE, offset += PAGE_SIZE, copySize -= PAGE_SIZE) {
    auto ptTable = MemManager::Instance().GetPTTable(_pml4Table, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
      throw upan::exception(XLOC, "page table not allocated at proces space address: 0x%lx", virtualAddress);
    }

    uint64_t realAddress = ptTable[ptIndex] & PAGE_MASK;
    memcpy((void*)realAddress, (void*)((uint64_t)processImage + offset), upan::min(copySize, (uint64_t)PAGE_SIZE));

    if (copySize <= PAGE_SIZE) {
      break;
    }
  }
}

void UserProcess::LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) {
  ElfParser mELFParser(szDLLName) ;

  uint64_t minMemAddr, maxMemAddr ;

  mELFParser.GetMemImageSize(minMemAddr, maxMemAddr) ;
  if(minMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC - DLL Min Address: %x", minMemAddr);

  unsigned uiDLLSectionSize ;
  upan::uniq_ptr<byte[]> bDLLSectionImage(ProcessLoader::Instance().LoadDLLInitSection(uiDLLSectionSize));

  const uint32_t uiDLLImageSize = ProcessLoader_GetCeilAlignedAddress(maxMemAddr - minMemAddr, 4) ;
  const uint32_t uiMemImageSize = uiDLLImageSize + uiDLLSectionSize ;
  const uint32_t uiNoOfPagesForDLL = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + DLL_ELF_SEC_HEADER_PAGE ;

  if(uiMemImageSize > MAX_PROCESS_SPACE_SIZE)
    throw upan::exception(XLOC, "DLL mem size %lu exceeds max limit per dll", uiMemImageSize, MAX_PROCESS_SPACE_SIZE);

  if(!KC::MKernelService().RequestDLLAlloCopy(uiNoOfPagesForDLL, szJustDLLName)) {
    throw upan::exception(XLOC, "Failed to allocate memory for DLL via kernal service");
  }

  ProcessDLLInfo& dllInfo = getDLLInfo(szJustDLLName).value();
  const uint64_t uiDLLLoadAddress = dllInfo.loadAddressForProcess();

  dllInfo.elfInfo()._elfSectionHeaders = mELFParser.CopyELFSectionHeader();
  dllInfo.elfInfo()._elfSecStrTable = mELFParser.CopyELFSecStrTable();

  upan::uniq_ptr<byte[]> bDLLImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] () { mELFParser.CopyProcessImage(bDLLImage.get(), 0, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bDLLImage.get() + uiDLLImageSize), (void*)bDLLSectionImage.get(), uiDLLSectionSize);

  // Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bDLLImage.get(), minMemAddr).onGood([&](uint64_t* uiGOT) {
    uiGOT[1] = dllInfo.id();
    uiGOT[2] = uiDLLImageSize + uiDLLLoadAddress;

    mELFParser.GetNoOfGOTEntries().onGood([&](uint32_t uiNoOfGOTEntries) {
      for(uint32_t i = 3; i < uiNoOfGOTEntries; i++)
        uiGOT[i] += uiDLLLoadAddress ;
    });
  });

/* Dynamic Relocation Entries are resolved here in Global Offset Table */
  mELFParser.GetSectionHeaderByTypeAndName(ElfSectionHeader::SHT_REL, REL_DYN_SUB_NAME).onGood([&] (Elf64_Shdr* pRelocationSectionHeader) {
    Elf64_Shdr *pDynamicSymSectiomHeader = mELFParser.GetSectionHeaderByIndex(
        pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);

    auto pELFDynRelTable = (ElfRelocSection::Elf64_Rel*)((uint64_t) bDLLImage.get() + pRelocationSectionHeader->sh_addr);
    unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize;

    auto pELFDynSymTable = (ElfSymbolTable::Elf64_Sym*)((uint64_t) bDLLImage.get() + pDynamicSymSectiomHeader->sh_addr);

    for (uint32_t i = 0; i < uiNoOfDynRelEntries; i++) {
      const auto uiRelType = ELF64_R_TYPE(pELFDynRelTable[i].r_info);

      if (uiRelType == ElfRelocSection::R_386_RELATIVE) {
        ((unsigned *) ((uint64_t) bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] += uiDLLLoadAddress;
      } else if (uiRelType == ElfRelocSection::R_386_GLOB_DAT) {
        ((unsigned *) ((uint64_t) bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] =
                pELFDynSymTable[ELF64_R_SYM(pELFDynRelTable[i].r_info)].st_value + uiDLLLoadAddress;
      }
    }
  });
  /* End of Dynamic Relocation Entries resolution */
  memcpy((void*) dllInfo.loadAddress(), bDLLImage.get(), uiMemImageSize);
}

void UserProcess::AllocateAddressSpace() {
  _pml4Table = (uint64_t*)(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  //Map kernel space into the process
  MemManager::Instance().MapAddressSpace(_pml4Table, 0x5, 0, 0, PROCESS_SPACE_FOR_OS);
  //Allocate process space
  MemManager::Instance().AllocateAddressSpace(_pml4Table, 0x7, _processBase + PROCESS_BASE, _processSpaceSize);

  _stackPDAddress = SchedulableProcess::Common::AllocateStackSpace();
}

void UserProcess::DeallocateResources() {
  DeallocateGUIFramebuffer();

  SchedulableProcess::Common::DeAllocateStackSpace(_stackPDAddress);
  MemManager::Instance().DeAllocatePhysicalPage(_stackPDAddress / PAGE_SIZE);

  SchedulableProcess::Common::SetStackPDTable(pdbr(), 0);

  MemManager::Instance().UnMapAddressSpace(pdbr(), 0, PROCESS_SPACE_FOR_OS);
  MemManager::Instance().DeallocateAddressSpace(pdbr());
  MemManager::Instance().DeAllocatePhysicalPage((uint64_t)pdbr() / PAGE_SIZE);
}

void UserProcess::MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) {
  const auto virtualDLLLoadAddress = PROCESS_DLL_START_ADDRESS + _totalNoOfPagesForDLL * PAGE_SIZE;
  MemManager::Instance().AllocateAddressSpace(pdbr(), 0x7, virtualDLLLoadAddress, noOfPagesForDLL * PAGE_SIZE);

  _loadedDLLs.push_back(dllName);
  _dllInfoMap.insert(DLLInfoMap::value_type(dllName, ProcessDLLInfo(_loadedDLLs.size() - 1, virtualDLLLoadAddress, noOfPagesForDLL)));

  _totalNoOfPagesForDLL += noOfPagesForDLL;
}

upan::option<ProcessDLLInfo&> UserProcess::getDLLInfo(const upan::string& dllName) {
  auto it = _dllInfoMap.find(dllName);
  if (it == _dllInfoMap.end()) {
    return upan::option<ProcessDLLInfo&>::empty();
  }
  return upan::option<ProcessDLLInfo&>(it->second);
}

upan::option<ProcessDLLInfo&> UserProcess::getDLLInfo(int id) {
  if (id < 0 || id >= _loadedDLLs.size()) {
    return upan::option<ProcessDLLInfo&>::empty();
  }
  return getDLLInfo(_loadedDLLs[id]);
}

void UserProcess::onLoad() {
  SchedulableProcess::Common::SwitchStack(pdbr(), _stackPDAddress);
}

void UserProcess::allocateGUIFramebuffer() {
  auto frameBufferAddress = GraphicsVideo::Instance().allocateFrameBuffer();
  MemManager::Instance().MapAddressSpace(pdbr(), 0x7,
                                    PROCESS_GUI_FRAMEBUFFER_ADDRESS,
                                    frameBufferAddress,
                                    GraphicsVideo::Instance().LFBPageCount() * PAGE_SIZE);
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

void UserProcess::DeallocateGUIFramebuffer() {
  if (_frame.get() != nullptr) {
    DMM_DeAllocateForKernel((uint64_t)_frame->frameBuffer().buffer());
    MemManager::Instance().UnMapAddressSpace(pdbr(), PROCESS_GUI_FRAMEBUFFER_ADDRESS, GraphicsVideo::Instance().LFBPageCount() * PAGE_SIZE);
    GraphicsVideo::Instance().removeFGProcess(processID());
  }
}