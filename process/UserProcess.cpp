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
#include <DynamicLinkLoader.h>
#include <MountManager.h>
#include <UserManager.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <DMM.h>
#include <GraphicsVideo.h>
#include <ElfDynamicSection.h>
#include <file_util.h>

using namespace ElfSectionHeader;
//using namespace ElfHeader ;
//using namespace ElfRelocSection ;
//using namespace ElfSymbolTable ;
using namespace ElfDynamicSection;

#define REL_DYN_SUB_NAME  ".rela.dyn"
#define BSS_SEC_NAME      ".bss"
#define DLL_ELF_SEC_HEADER_PAGE 1

UserProcess::UserProcess(const upan::string &name, int parentID, int userID,
                         bool isFGProcess, int noOfParams, char** args)
    : AutonomousProcess(name, parentID, isFGProcess), _iodTable(_processID, parentID) {
  _mainThreadID = _processID;
  _pml4Table = nullptr;
  Load(noOfParams, args);
  _totalNoOfPagesForDLL = 0;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
  _userID = userID == DERIVE_FROM_PARENT && !parentProcess.isEmpty() ? parentProcess.value().userID() : _userID;
}

UserThread& UserProcess::CreateThread(uintptr_t threadCaller, uintptr_t entryAddress, void* arg) {
  return *new UserThread(*this, threadCaller, entryAddress, arg);
}

void UserProcess::Load(int numOfParams, char** argvList) {
  ElfParser mELFParser(_name.c_str());

  uint64_t minMemAddr, maxMemAddr;

  mELFParser.GetMemImageSize(minMemAddr, maxMemAddr);
  if(minMemAddr < USER_PROCESS_MIN_LOAD_ADDRESS)
    throw upan::exception(XLOC, "process min load address %x is less than USER_PROCESS_MIN_LOAD_ADDRESS %x", minMemAddr, USER_PROCESS_MIN_LOAD_ADDRESS);

  if((minMemAddr % PAGE_SIZE) != 0)
    throw upan::exception(XLOC, "process min load address %x is not page aligned", minMemAddr);

  uint64_t processImageSize = upan::align(maxMemAddr - minMemAddr, 8);
  _processSpaceSize = processImageSize + DynamicLinkLoader::Instance().dllResolverSize();

  _processBase = minMemAddr;

  if(_processSpaceSize > MAX_PROCESS_SPACE_SIZE) {
    throw upan::exception(XLOC, "process requires %lu space that's larger than supported %lu", _processSpaceSize, MAX_PROCESS_SPACE_SIZE);
  }

  AllocateAddressSpace();
  _elfInfo._elfSectionHeaders = mELFParser.CopyELFSectionHeader();
  _elfInfo._elfSecStrTable = mELFParser.CopyELFSecStrTable();

  upan::uniq_ptr<byte[]> bProcessImage(new byte[sizeof(char) * _processSpaceSize]);

  upan::trycall([&] { mELFParser.CopyProcessImage(bProcessImage.get(), _processBase, processImageSize); }).onBad([&] (const upan::error& err) {
    DeallocateResources();
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bProcessImage.get() + processImageSize),
         DynamicLinkLoader::Instance().dllResolverProgBits(),
         DynamicLinkLoader::Instance().dllResolverSize());

  // Setting the Dynamic Link Loader Address in GOT
  mELFParser.GetGOTAddress(bProcessImage.get(), minMemAddr).onGood([&](uint64_t* uiGOT) {
    uiGOT[1] = -1;
    uiGOT[2] = minMemAddr + processImageSize;
  });

  // initialize BSS segment to 0
  mELFParser.GetSectionHeaderByTypeAndName(ElfSectionHeader::SHT_NOBITS, BSS_SEC_NAME).onGood([&] (Elf64_Shdr* bssSectionHeader) {
    void* bss = (void*) (bProcessImage.get() + bssSectionHeader->sh_addr - minMemAddr);
    memset(bss, 0, bssSectionHeader->sh_size);
  });

  CopyElfImage(bProcessImage.get(), _processSpaceSize, _processBase);

  const auto stackTopAddress = PushProgramInitStackData(numOfParams, argvList);
  const auto entryAdddress = mELFParser.GetProgramStartAddress();

  _tlsp.reset(new ThreadLocalSpace());
  if (mELFParser.GetTLSTotalSize()) {
    _tlsp->add(mELFParser.GetTLSTotalSize(), mELFParser.GetTLSInitImageSize(), mELFParser.GetTLSInitImage());
  }

  _tls.reset(new ThreadLocalStorage(_processID, _pml4Table, tlsp(), 0x7));

  LoadDLLs(mELFParser, bProcessImage.get());

  _taskContext.rdi = numOfParams; //argc
  _taskContext.rsi = stackTopAddress; //argv

  _taskContext.interruptState.cs = USER_CODE_SELECTOR | 0x3;
  _taskContext.interruptState.rip = entryAdddress;
  _taskContext.interruptState.ss = USER_DATA_SELECTOR | 0x3;
  _taskContext.interruptState.rsp = stackTopAddress;
  _taskContext.interruptState.rflags = 0x202;
}

uint64_t UserProcess::PushProgramInitStackData(int numOfParams, char **argvList) {
  const unsigned argvEntriesSize = numOfParams * sizeof(uint64_t); // address of char* entry (second dimension) of argv array

  uint32_t argumentSize = 0;
  for(int i = 0; i < numOfParams; i++) {
    argumentSize += (strlen(argvList[i]) + 1);
  }

  //The stack must be aligned to 16 byte otherwise SSE/SSE2/SSE3 instructions will cause General Protection Fault
  const uint32_t processEntryStackSize = upan::align(argvEntriesSize + argumentSize, 16);
  if (processEntryStackSize > PROCESS_INIT_STACK_SIZE) {
    throw upan::exception(XLOC, "Startup arguments size is larger than reserved init stack size of %u", PROCESS_INIT_STACK_SIZE);
  }

  const uint64_t virtualStackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_SYSCALL_STACK_SIZE - processEntryStackSize;
  const uintptr_t realStackTopAddress = MemManager::Instance().GetFlatAddressFromPD((uint64_t*)_stackPDAddress, virtualStackTopAddress);

  argumentSize = 0 ;// argv[0] through argv[argc - 1]
  for(int i = 0; i < numOfParams; i++) {
    const uint64_t realArgAddress = realStackTopAddress + argvEntriesSize + argumentSize;
    const uint64_t virtualArgAddress = virtualStackTopAddress + argvEntriesSize + argumentSize;
    //first dimension of argv
    ((uint64_t*)realStackTopAddress)[i] = virtualArgAddress;

    //second dimension of argv
    strcpy((char*)realArgAddress, argvList[i]);
    argumentSize += (strlen(argvList[i]) + 1);
  }

  return virtualStackTopAddress;
}

void UserProcess::CopyElfImage(const uint8_t* processImage, int imageSize, uint64_t virtualLoadAddress) {
  uint64_t virtualAddress = virtualLoadAddress;
  const uint64_t endAddress = virtualAddress + imageSize;

  for(uint64_t offset = 0, copySize = imageSize;
    virtualAddress < endAddress;
    virtualAddress += PAGE_SIZE, offset += PAGE_SIZE, copySize -= PAGE_SIZE) {
    auto ptTable = MemManager::Instance().GetPTTable(_pml4Table, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
      throw upan::exception(XLOC, "page table not allocated at process space address: 0x%lx", virtualAddress);
    }

    uint64_t realAddress = ptTable[ptIndex] & PAGE_MASK;
    memcpy((void*)realAddress, (void*)((uint64_t)processImage + offset), upan::min(copySize, (uint64_t)PAGE_SIZE));

    if (copySize <= PAGE_SIZE) {
      break;
    }
  }
}

void UserProcess::LoadDLLs(ElfParser& elfParser, uint8_t* processImage) {
  const auto dynSectionHeader = elfParser.GetSectionHeaderByType(SHT_DYNAMIC).goodValueOrThrow(XLOC);
  const auto noOfEntries = dynSectionHeader->sh_size / dynSectionHeader->sh_entsize;
  const auto dynSection = (Elf64_Dyn*)(processImage + dynSectionHeader->sh_addr - _processBase) ;
  const auto dynSymStrSectionHeader = elfParser.GetSectionHeaderByIndex(dynSectionHeader->sh_link).goodValueOrThrow(XLOC);
  const auto dynSymStrTable = (const char*)(processImage + dynSymStrSectionHeader->sh_addr - _processBase) ;

  for(uint32_t i = 0; i < noOfEntries; ++i) {
    if (dynSection[i].d_tag == DT_NEEDED) {
      const auto dllName = (char*)&dynSymStrTable[ dynSection[i].d_un.d_val ];
      printf("\n loading dll: %s", dllName);
      LoadELFDLL(dllName);
    }
  }
}

void UserProcess::LoadELFDLL(const upan::string& dllName) {
  if(!getDLLInfo(dllName).isEmpty()) {
    return;
  }

  auto dllPath = upan::file_path::resolve(dllName, LD_LIBRARY_PATH_ENV, LIB_PATH);
  if (dllPath.isEmpty()) {
    throw upan::exception(XLOC, "DLL shared object file not found: %s", dllName.c_str());
  }

  ElfParser mELFParser(dllPath.value());

  uint64_t minMemAddr, maxMemAddr ;
  mELFParser.GetMemImageSize(minMemAddr, maxMemAddr) ;
  if(minMemAddr != 0)
    throw upan::exception(XLOC, "Not a PIC - DLL Min Address: %x", minMemAddr);

  const uint32_t uiDLLImageSize = upan::align(maxMemAddr - minMemAddr, 4) ;
  const uint32_t uiMemImageSize = uiDLLImageSize + DynamicLinkLoader::Instance().dllResolverSize();
  const uint32_t uiNoOfPagesForDLL = MemManager::Instance().GetProcessSizeInPages(uiMemImageSize) + DLL_ELF_SEC_HEADER_PAGE ;

  if(uiMemImageSize > MAX_PROCESS_SPACE_SIZE)
    throw upan::exception(XLOC, "DLL mem size %lu exceeds max limit per dll", uiMemImageSize, MAX_PROCESS_SPACE_SIZE);

  MapDLLPagesToProcess(uiNoOfPagesForDLL, dllName);

  ProcessDLLInfo& dllInfo = getDLLInfo(dllName).value();
  const uint64_t uiDLLLoadAddress = dllInfo.loadAddressForProcess();

  dllInfo.setElfInfo(mELFParser.CopyELFSectionHeader(), mELFParser.CopyELFSecStrTable());

  upan::uniq_ptr<byte[]> bDLLImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] () { mELFParser.CopyProcessImage(bDLLImage.get(), 0, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bDLLImage.get() + uiDLLImageSize),
         DynamicLinkLoader::Instance().dllResolverProgBits(),
         DynamicLinkLoader::Instance().dllResolverSize());

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
  mELFParser.GetSectionHeaderByTypeAndName(ElfSectionHeader::SHT_RELA, REL_DYN_SUB_NAME).onGood([&] (Elf64_Shdr* pRelocationSectionHeader) {
    Elf64_Shdr *pDynamicSymSectiomHeader = mELFParser.GetSectionHeaderByIndex(pRelocationSectionHeader->sh_link).goodValueOrThrow(XLOC);

    auto pELFDynRelTable = (ElfRelocSection::Elf64_Rela*)((uint64_t) bDLLImage.get() + pRelocationSectionHeader->sh_addr);
    unsigned uiNoOfDynRelEntries = pRelocationSectionHeader->sh_size / pRelocationSectionHeader->sh_entsize;

    auto pELFDynSymTable = (ElfSymbolTable::Elf64_Sym*)((uint64_t) bDLLImage.get() + pDynamicSymSectiomHeader->sh_addr);

    for (uint32_t i = 0; i < uiNoOfDynRelEntries; i++) {
      const auto uiRelType = ELF64_R_TYPE(pELFDynRelTable[i].r_info);

      if (uiRelType == ElfRelocSection::R_X86_64_RELATIVE) {
        ((uint64_t*)((uint64_t) bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] += uiDLLLoadAddress;
      } else if (uiRelType == ElfRelocSection::R_X86_64_GLOB_DAT) {
        ((uint64_t*)((uint64_t) bDLLImage.get() + pELFDynRelTable[i].r_offset))[0] =
                pELFDynSymTable[ELF64_R_SYM(pELFDynRelTable[i].r_info)].st_value + uiDLLLoadAddress + pELFDynRelTable[i].r_addend;
      }
    }
  });

  /* End of Dynamic Relocation Entries resolution */
  CopyElfImage(bDLLImage.get(), uiMemImageSize, dllInfo.loadAddress());
  //memcpy((void*) dllInfo.loadAddress(), bDLLImage.get(), uiMemImageSize);
}

void UserProcess::AllocateAddressSpace() {
  _pml4Table = (uint64_t*)(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);

  //Map kernel space into the process
  //The first PDP entry = 1 GB of memory is reserved for kernel space
  auto pdpPage = (uint64_t*)(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  auto kernelPdpPage = PAGE_TABLE(MEM_PML4_TABLE, 0);
  pdpPage[0] = PAGE_ADDRESS(kernelPdpPage, 0) | 0x7;
 _pml4Table[0] = (uint64_t)pdpPage | 0x7;

  //Allocate process space
  MemManager::Instance().AllocateAddressSpace(_pml4Table, 0x7, _processBase, _processSpaceSize);

  _stackPDAddress = SchedulableProcess::Common::AllocateStackSpace();
}

void UserProcess::DeallocateResources() {
  DeallocateGUIFramebuffer();

  SchedulableProcess::Common::DeAllocateStackSpace(_stackPDAddress);
  MemManager::Instance().DeAllocatePhysicalPage(_stackPDAddress / PAGE_SIZE);

  SchedulableProcess::Common::SetStackPDTable(pml4Table(), 0);

  //Threads and main-thread/process share common page-table (CR3)
  //Therefore, the allocated pages of page-table are de-allocated only in the main-thread/process

  //release the PDP that's mapped to Kernel space
  auto pdpPage = PAGE_TABLE(_pml4Table, 0);
  pdpPage[0] = 0;
  MemManager::Instance().DeallocateAddressSpace(pml4Table());
}

void UserProcess::MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) {
  const auto virtualDLLLoadAddress = PROCESS_DLL_START_ADDRESS + _totalNoOfPagesForDLL * PAGE_SIZE;
  //printf("\n DLL Addr: %llx, %d", virtualDLLLoadAddress, noOfPagesForDLL);
  MemManager::Instance().AllocateAddressSpace(pml4Table(), 0x7, virtualDLLLoadAddress, noOfPagesForDLL * PAGE_SIZE);
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
  SchedulableProcess::Common::SwitchStack(pml4Table(), _stackPDAddress);
}

void UserProcess::allocateGUIFramebuffer() {
  auto frameBufferAddress = GraphicsVideo::Instance().allocateFrameBuffer();
  MemManager::Instance().MapAddressSpace(pml4Table(), 0x7,
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
    KernelDMM::Instance().free((uint64_t)_frame->frameBuffer().buffer());
    MemManager::Instance().UnMapAddressSpace(pml4Table(), PROCESS_GUI_FRAMEBUFFER_ADDRESS, GraphicsVideo::Instance().LFBPageCount() * PAGE_SIZE);
    GraphicsVideo::Instance().removeFGProcess(processID());
  }
}