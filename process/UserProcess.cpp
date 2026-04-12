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
#include <UserManager.h>
#include <ElfParser.h>
#include <ElfRelocationSection.h>
#include <ElfSymbolTable.h>
#include <DMM.h>
#include <GraphicsVideo.h>
#include <ElfDynamicSection.h>
#include <file_util.h>

using namespace ElfSectionHeader;
using namespace ElfSymbolTable;
using namespace ElfDynamicSection;

#define REL_DYN_SUB_NAME  ".rela.dyn"
#define BSS_SEC_NAME      ".bss"

UserProcess::UserProcess(const upan::string &name, int parentID, int userID, bool isFGProcess,
                         const upan::vector<upan::string>& argv,
                         const upan::vector<upan::string>& envp)
                         : AutonomousProcess(name, parentID, isFGProcess), _totalNoOfPagesForDLL(0), _pml4Table(nullptr) {
  Load(argv, envp);

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
  _userID = userID == DERIVE_FROM_PARENT && !parentProcess.isEmpty() ? parentProcess.value().userID() : _userID;
}

UserProcess::UserProcess(UserProcess& parent)
  : AutonomousProcess(parent.name(), parent.processID(), parent.isFGProcessGroup()),
    _totalNoOfPagesForDLL(0), _pml4Table(nullptr) {
  _userID = parent.userID();
  LoadFromParent(parent);
  parent.addChildProcessID(_processID);
  _taskContext = parent._taskContext;
}

UserThread& UserProcess::CreateThread(uintptr_t threadCaller, uintptr_t entryAddress, void* arg, bool joinable) {
  return *new UserThread(*this, threadCaller, entryAddress, arg, joinable);
}

void UserProcess::Load(const upan::vector<upan::string>& argv, const upan::vector<upan::string>& envp) {
  ElfParser mELFParser(_name.c_str());

  uint64_t minMemAddr, maxMemAddr;

  mELFParser.GetMemImageSize(minMemAddr, maxMemAddr);
  if(minMemAddr < USER_PROCESS_MIN_LOAD_ADDRESS)
    throw upan::exception(XLOC, "process min load address %x is less than USER_PROCESS_MIN_LOAD_ADDRESS %x", minMemAddr, USER_PROCESS_MIN_LOAD_ADDRESS);

  if((minMemAddr % PAGE_SIZE) != 0)
    throw upan::exception(XLOC, "process min load address %x is not page aligned", minMemAddr);

  uint64_t processImageSize = upan::align_up(maxMemAddr - minMemAddr, 8);
  _processSpaceSize = processImageSize + DynamicLinkLoader::Instance().dllResolverSize();

  _processBase = minMemAddr;

  if(_processSpaceSize > MAX_PROCESS_SPACE_SIZE) {
    throw upan::exception(XLOC, "process requires %lu space that's larger than supported %lu", _processSpaceSize, MAX_PROCESS_SPACE_SIZE);
  }

  AllocateAddressSpace();

  upan::uniq_ptr<byte[]> bProcessImage(new byte[sizeof(char) * _processSpaceSize]);
  upan::trycall([&] { mELFParser.CopyProcessImage(bProcessImage.get(), _processBase, processImageSize); }).onBad([&] (const upan::error& err) {
    DeallocateResources();
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bProcessImage.get() + processImageSize),
         DynamicLinkLoader::Instance().dllResolverProgBits(),
         DynamicLinkLoader::Instance().dllResolverSize());

  //sh_addr in elf section headers will be minMemAddr + offset. Therefore, in order to reference the correct offset
  //in allocated processImage heap space, subtract minMemAddr from the processImage heap allocated address, so _base + sh_addr => processImageBase + offset
  _elfInfo.init((uint64_t)bProcessImage.get() - minMemAddr, mELFParser.CopyELFSectionHeader(), mELFParser.CopyELFSecStrTable());

  // Setting the Dynamic Link Loader Address in GOT
  _elfInfo.getGOT().ifPresent([&](ELFInfo::Section& gotSection){
    auto got = gotSection.get<uint64_t>();
    got[1] = -1;
    got[2] = minMemAddr + processImageSize;
  });

  // initialize BSS segment to 0
  _elfInfo.getSectionByTypeAndName(ElfSectionHeader::SHT_NOBITS, BSS_SEC_NAME).onGood([&](ELFInfo::Section& section) {
    memset(section.get<void*>(), 0, section.size());
  });

  _tlsp.reset(new ThreadLocalSpace());
  if (mELFParser.GetTLSTotalSize()) {
    _tlsp->add(mELFParser.GetTLSTotalSize(), mELFParser.GetTLSInitImageSize(), mELFParser.GetTLSInitImage());
  }

  LoadDLLs();

  _tls.reset(new ThreadLocalStorage(_processID, false, _pml4Table, tlsp(), 0x7));

  CopyElfImage(bProcessImage.get(), _processSpaceSize, _processBase);

  const auto stackTopAddress = PushProgramInitStackData(argv, envp);
  const auto entryAdddress = mELFParser.GetProgramStartAddress();

  auto relocateInfoConsumer = [this](const char* symName, const Elf64_Addr value) {
    _relocateInfoExeMap.insert(RELOCATE_INFO_EXE_MAP::value_type (symName, ExeRelocateInfo(0, value)));
  };
  _elfInfo.extractDynSymbols(relocateInfoConsumer);

  _elfInfo.adjustBase(0);

  _taskContext.rdi = argv.size(); //argc
  _taskContext.rsi = stackTopAddress; //argv

  _taskContext.interruptState.cs = USER_CODE_SELECTOR | 0x3;
  _taskContext.interruptState.rip = entryAdddress;
  _taskContext.interruptState.ss = USER_DATA_SELECTOR | 0x3;
  _taskContext.interruptState.rsp = stackTopAddress;
  _taskContext.interruptState.rflags = 0x202;
}

uint64_t UserProcess::PushProgramInitStackData(const upan::vector<upan::string>& argv, const upan::vector<upan::string>& envp) {
  const uint32_t argvD1Size = argv.size() * sizeof(uintptr_t); // address of char* entry (second dimension) of argv array
  uint32_t argvD2Size = 0;
  for(const auto& i : argv) {
    argvD2Size += i.length() + 1;
  }

  const uint32_t envpD1Size = (envp.size() +  1) * sizeof(uintptr_t); // no. of envp entries + 1 for null terminator
  uint32_t envpD2Size = 0;
  for(const auto& e : envp) {
    envpD2Size += e.length() + 1;
  }

  //The stack must be aligned to 16 byte otherwise SSE/SSE2/SSE3 instructions will cause General Protection Fault
  const uint32_t processEntryStackSize = upan::align_up(argvD1Size + argvD2Size + envpD1Size + envpD2Size, 16);
  if (processEntryStackSize > PROCESS_INIT_STACK_SIZE) {
    throw upan::exception(XLOC, "Startup arguments size is larger than reserved init stack size of %u", PROCESS_INIT_STACK_SIZE);
  }

  const uint64_t virtualStackTopAddress = PROCESS_STACK_TOP_ADDRESS - processEntryStackSize;
  const uintptr_t realStackTopAddress = MemManager::Instance().GetFlatAddressFromPD((uint64_t*)_stackPDAddress, virtualStackTopAddress);

  uint32_t pos = argvD1Size;// argv[0] through argv[argc - 1]
  for(int i = 0; i < argv.size(); i++) {
    const uint64_t realArgAddress = realStackTopAddress + pos;
    const uint64_t virtualArgAddress = virtualStackTopAddress + pos;
    //first dimension of argv
    ((uint64_t*)realStackTopAddress)[i] = virtualArgAddress;

    //second dimension of argv
    strcpy((char*)realArgAddress, argv[i].c_str());
    pos += argv[i].length() + 1;
  }

  const uint64_t realEnvpStackTopAddress = realStackTopAddress + pos;
  const uint64_t virtualEnvpStackTopAddress = virtualStackTopAddress + pos;

  pos = envpD1Size;
  for(int i = 0; i < envp.size(); i++) {
    const uint64_t realArgAddress = realEnvpStackTopAddress + pos;
    const uint64_t virtualArgAddress = virtualEnvpStackTopAddress + pos;
    //first dimension of envp
    ((uint64_t*)realEnvpStackTopAddress)[i] = virtualArgAddress;

    //second dimension of envp
    strcpy((char*)realArgAddress, envp[i].c_str());
    pos += envp[i].length() + 1;
  }
  //null termination of envp
  ((uint64_t*)realEnvpStackTopAddress)[envp.size()] = 0;

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

void UserProcess::LoadDLLs() {
  _elfInfo.getDynSection().ifPresent([&](const Elf64_Dyn* dynSection) {
    for (Elf64_Xword i = 0; i < _elfInfo.getDynSectionSize(); ++i) {
      if (dynSection[i].d_tag == DT_NEEDED) {
        LoadELFDLL(_elfInfo.getDynSymName(dynSection[i].d_un.d_val));
      }
    }
  });
}

void UserProcess::LoadELFDLL(const upan::string& dllName) {
  if(!getDLLInfo(dllName).isEmpty()) {
    return;
  }

  auto dllPath = upan::file_path::resolve(dllName, LD_LIBRARY_PATH_ENV, LIB_PATH);
  if (dllPath.isEmpty()) {
    throw upan::exception(XLOC, "DLL shared object file not found: %s", dllName.c_str());
  }

  ElfParser dllElfParser(dllPath.value());

  uint64_t minMemAddr, maxMemAddr ;
  dllElfParser.GetMemImageSize(minMemAddr, maxMemAddr) ;
  if(minMemAddr != 0) {
    throw upan::exception(XLOC, "Not a PIC - DLL Min Address: %x", minMemAddr);
  }

  const uint32_t uiDLLImageSize = upan::align_up(maxMemAddr - minMemAddr, 4) ;
  const uint32_t uiMemImageSize = uiDLLImageSize + DynamicLinkLoader::Instance().dllResolverSize();
  const uint32_t uiNoOfPagesForDLL = MemManager::GetProcessSizeInPages(uiMemImageSize);

  if(uiMemImageSize > MAX_PROCESS_SPACE_SIZE) {
    throw upan::exception(XLOC, "DLL mem size %lu exceeds max limit per dll", uiMemImageSize, MAX_PROCESS_SPACE_SIZE);
  }

  MapDLLPagesToProcess(uiNoOfPagesForDLL, dllName);

  DLLInfo& dllInfo = getDLLInfo(dllName).value();
  const uint64_t uiDLLLoadAddress = dllInfo.virtualLoadAddress();

  upan::uniq_ptr<byte[]> bDLLImage(new byte[sizeof(char) * uiMemImageSize]);

  upan::trycall([&] () { dllElfParser.CopyProcessImage(bDLLImage.get(), 0, uiMemImageSize); }).onBad([&] (const upan::error& err) {
    throw upan::exception(XLOC, err);
  });

  memcpy((void*)(bDLLImage.get() + uiDLLImageSize),
         DynamicLinkLoader::Instance().dllResolverProgBits(),
         DynamicLinkLoader::Instance().dllResolverSize());

  dllInfo.elfInfo().init((uint64_t)bDLLImage.get(),
                     dllElfParser.CopyELFSectionHeader(),
                     dllElfParser.CopyELFSecStrTable());

  // Setting the Dynamic Link Loader Address in GOT
  dllInfo.elfInfo().getGOT().ifPresent([&](ELFInfo::Section& gotSection) {
    auto got = gotSection.get<uint64_t>();
    got[1] = dllInfo.id();
    got[2] = uiDLLImageSize + uiDLLLoadAddress;

    for (uint32_t i = 3; i < gotSection.size(); i++)
      got[i] += uiDLLLoadAddress;
  });

  if (dllElfParser.GetTLSTotalSize()) {
    const auto& tlsInfo = _tlsp->add(dllElfParser.GetTLSTotalSize(), dllElfParser.GetTLSInitImageSize(), dllElfParser.GetTLSInitImage());
    dllInfo.setTLSInfo(tlsInfo.moduleId(), tlsInfo.offset());
  }

  auto relocateInfoConsumer = [this, &dllInfo](const char* symName, const Elf64_Addr value) {
    _relocateInfoDLLMap.insert(RELOCATE_INFO_DLL_MAP::value_type (symName, DLLRelocateInfo(dllInfo, value)));
  };
  dllInfo.elfInfo().extractDynSymbols(relocateInfoConsumer);

/* Dynamic Relocation Entries are resolved here in Global Offset Table */
  dllInfo.elfInfo().getDynRelTable().ifPresent([&](Elf64_Rela* dynRelTable) {
    for (Elf64_Xword i = 0; i < dllInfo.elfInfo().getDynRelTableSize(); ++i) {
      const auto uiRelType = ELF64_R_TYPE(dynRelTable[i].r_info);
      const auto dynSymTable = dllInfo.elfInfo().getDynSymTable().valueOrThrow(XLOC, "dynSymTable not found");

      if (uiRelType == ElfRelocSection::R_X86_64_RELATIVE) {
        ((uint64_t*)((uint64_t) bDLLImage.get() + dynRelTable[i].r_offset))[0] += uiDLLLoadAddress;
      } else if (uiRelType == ElfRelocSection::R_X86_64_GLOB_DAT || uiRelType == ElfRelocSection::R_X86_64_64) {
        ((uint64_t*)((uint64_t) bDLLImage.get() + dynRelTable[i].r_offset))[0] =
                dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)].st_value + uiDLLLoadAddress + dynRelTable[i].r_addend;
      }
    }
  });

  /* End of Dynamic Relocation Entries resolution */
  CopyElfImage(bDLLImage.get(), uiMemImageSize, dllInfo.virtualLoadAddress());

  dllInfo.elfInfo().adjustBase(dllInfo.virtualLoadAddress());
  //memcpy((void*) dllInfo.loadAddress(), bDLLImage.get(), uiMemImageSize);
}

void UserProcess::LoadFromParent(UserProcess& parent) {
  _processSpaceSize = parent._processSpaceSize;
  _processBase = parent._processBase;

  AllocateAndCopyAddressSpaceFromParent(parent);

  _elfInfo = parent._elfInfo;

//  upan::uniq_ptr<RootFrame> _frame;

  //this is copying over all TLS sections, that includes those from DLLs
  _tlsp.reset(new ThreadLocalSpace());
  if (!parent._tlsp->getDTV().empty()) {
    for (const auto& dtv : parent._tlsp->getDTV()) {
      _tlsp->add(dtv.total_len, dtv.init_len, dtv.init_image);
    }
  }

  _relocateInfoExeMap = parent._relocateInfoExeMap;

  LoadDLLsFromParent(parent);

  _tls.reset(new ThreadLocalStorage(_processID, false, _pml4Table, tlsp(), 0x7));
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

void UserProcess::AllocateAndCopyAddressSpaceFromParent(UserProcess& parent) {
  _pml4Table = (uint64_t*)(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);

  //Map kernel space into the process
  //The first PDP entry = 1 GB of memory is reserved for kernel space
  auto pdpPage = (uint64_t*)(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  auto kernelPdpPage = PAGE_TABLE(MEM_PML4_TABLE, 0);
  pdpPage[0] = PAGE_ADDRESS(kernelPdpPage, 0) | 0x7;
  _pml4Table[0] = (uint64_t)pdpPage | 0x7;

  //Allocate process space
  MemManager::Instance().AllocateAndCopyAddressSpace(_pml4Table, parent._pml4Table, 0x7,
                                                     parent._processBase, _processSpaceSize);
  MemManager::Instance().AllocateAndCopyAddressSpace(_pml4Table, parent._pml4Table, 0x7,
                                                     PROCESS_HEAP_START_ADDRESS, PROCESS_HEAP_SIZE);
  MemManager::Instance().AllocateAndCopyAddressSpace(_pml4Table, parent._pml4Table, 0x7,
                                                     PROCESS_STACK_BASE - PROCESS_STACK_SIZE, PROCESS_STACK_SIZE);

  _stackPDAddress = MemManager::Instance().GetFlatPDAddress(_pml4Table, PROCESS_STACK_BASE - 1);
}

void UserProcess::LoadDLLsFromParent(UserProcess& parent) {
  _dllInfoMap = parent._dllInfoMap;
  upan::map<int, const DLLInfo&> localDllInfoMap;
  for(const auto& dllInfo : _dllInfoMap) {
    localDllInfoMap.insert(upan::map<int, const DLLInfo&>::value_type(dllInfo.second.id(), dllInfo.second));
    MemManager::Instance().AllocateAndCopyAddressSpace(_pml4Table, parent._pml4Table, 0x7,
                                                       dllInfo.second.virtualLoadAddress(), dllInfo.second.noOfPages() * PAGE_SIZE);
  }
  _totalNoOfPagesForDLL = parent._totalNoOfPagesForDLL;

  for(const auto& e : parent._relocateInfoDLLMap) {
    auto& dllInfo = localDllInfoMap.find(e.second.dllInfo().id())->second;
    _relocateInfoDLLMap.insert(RELOCATE_INFO_DLL_MAP::value_type (e.first, DLLRelocateInfo(dllInfo, e.second.value())));
  }
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
  //printf("\n DLL Addr: %llx, %d, %s", virtualDLLLoadAddress, noOfPagesForDLL, dllName.c_str());
  MemManager::Instance().AllocateAddressSpace(pml4Table(), 0x7, virtualDLLLoadAddress, noOfPagesForDLL * PAGE_SIZE);
  _dllInfoMap.insert(DLLInfoMap::value_type(dllName, DLLInfo(_dllInfoMap.size(), virtualDLLLoadAddress, noOfPagesForDLL)));
  _totalNoOfPagesForDLL += noOfPagesForDLL;
}

void UserProcess::setupSignalStackFrame(const struct sigaction& action, const Signal& signal) {
  SchedulableProcess::Common::SetupSignalStackFrame(*this, _stackPDAddress, action, signal);
}

upan::option<DLLInfo&> UserProcess::getDLLInfo(const upan::string& dllName) {
  auto it = _dllInfoMap.find(dllName);
  if (it == _dllInfoMap.end()) {
    return upan::option<DLLInfo&>::empty();
  }
  return upan::option<DLLInfo&>(it->second);
}

upan::option<DLLInfo&> UserProcess::getDLLInfo(int id) {
  for(auto& i : _dllInfoMap) {
    if (i.second.id() == id) {
      return upan::option<DLLInfo&>(i.second);
    }
  }
  return upan::option<DLLInfo&>::empty();
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

upan::option<IRelocateInfo&> UserProcess::getRelocateInfo(const upan::string& symName, bool fallbackToExe, int stBind) {
  auto it = _relocateInfoDLLMap.find(symName);
  if (it == _relocateInfoDLLMap.end()) {
    if (fallbackToExe) {
      auto et = _relocateInfoExeMap.find(symName);
      if (et != _relocateInfoExeMap.end()) {
        return upan::option<IRelocateInfo&>(et->second);
      }
    }
    if (stBind != STB_WEAK) {
      printf("\n Dynamic Symbol %s not found in any DLL/Exe, relocation failed!", symName.c_str());
    }
    return upan::option<IRelocateInfo&>::empty();
  }
  return upan::option<IRelocateInfo&>(it->second);
}

process_init_fini_t* UserProcess::initRelocate() {
  const int init_fini_size = _dllInfoMap.size() + 1 /* for the main executable */ + 1 /* for the terminator */;
  auto init_fini_list = (process_init_fini_t*)_dmm.allocate(sizeof(process_init_fini_t) * init_fini_size, sizeof(uintptr_t));

  memset(init_fini_list, 0, sizeof(process_init_fini_t) * init_fini_size);

  relocateMainExe(init_fini_list[init_fini_size - 2]);
  relocateDLLs(init_fini_list);

  init_fini_list[init_fini_size - 1]._end = true;

  return init_fini_list;
}

//relocate dynamic symbols used within the main process space
void UserProcess::relocateMainExe(process_init_fini_t& init_fini) {
  _elfInfo.getDynRelTable().ifPresent([&](Elf64_Rela* dynRelTable) {
    auto dynSymTable = _elfInfo.getDynSymTable().valueOrThrow(XLOC, "no dynamic symbol table found");

    for (Elf64_Xword i = 0; i < _elfInfo.getDynRelTableSize(); ++i) {
      const auto relType = ELF64_R_TYPE(dynRelTable[i].r_info);

      if (relType == ElfRelocSection::R_X86_64_TPOFF64) {
        const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
        const char* symName = _elfInfo.getDynSymName(dynSym.st_name);
        auto rel_offset = (uint64_t*) dynRelTable[i].r_offset;
        getRelocateInfo(symName, false, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
          *rel_offset = relocateInfo.value() + dynRelTable[i].r_addend - relocateInfo.tlsOffset();
        });
      } else if (relType == ElfRelocSection::R_X86_64_COPY) {
        const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
        const char* symName = _elfInfo.getDynSymName(dynSym.st_name);
        getRelocateInfo(symName, false, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
          auto src = (void*)GLOBAL_REL_ADDR(relocateInfo.value() + dynRelTable[i].r_addend, relocateInfo.base());
          auto dest = (void*)dynRelTable[i].r_offset;
          memcpy(dest, src, dynSym.st_size);
        });
      } else if (relType == ElfRelocSection::R_X86_64_GLOB_DAT || relType == ElfRelocSection::R_X86_64_64) {
        const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
        const char* symName = _elfInfo.getDynSymName(dynSym.st_name);
        auto rel_offset = (uint64_t*)dynRelTable[i].r_offset;
        getRelocateInfo(symName, false, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
            *rel_offset = GLOBAL_REL_ADDR(relocateInfo.value() + dynRelTable[i].r_addend, relocateInfo.base());
        });
      }
    }
  });

  _elfInfo.loadInitFini(init_fini);
}

void UserProcess::relocateDLLs(process_init_fini_t* init_fini_list) {
  int init_fini_index = 0;

  for (auto& dllEntry: _dllInfoMap) {
    auto& elfInfo = dllEntry.second.elfInfo();
    elfInfo.getDynRelTable().ifPresent([&](Elf64_Rela* dynRelTable) {
      auto dynSymTable = elfInfo.getDynSymTable().valueOrThrow(XLOC, "no dynamic symbol table found");

      for (Elf64_Xword i = 0; i < elfInfo.getDynRelTableSize(); ++i) {
        const auto relType = ELF64_R_TYPE(dynRelTable[i].r_info);

        if (relType == ElfRelocSection::R_X86_64_DTPMOD64) {
          const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
          const char* symName = elfInfo.getDynSymName(dynSym.st_name);
          auto rel_offset = (uint64_t*)GLOBAL_REL_ADDR(dynRelTable[i].r_offset, elfInfo.getBase());
          getRelocateInfo(symName, false, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
            *rel_offset = relocateInfo.tlsModuleId();
          });
        } else if (relType == ElfRelocSection::R_X86_64_DTPOFF64) {
          const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
          const char* symName = elfInfo.getDynSymName(dynSym.st_name);
          auto rel_offset = (uint64_t*)GLOBAL_REL_ADDR(dynRelTable[i].r_offset, elfInfo.getBase());
          getRelocateInfo(symName, false, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
            *rel_offset = relocateInfo.value() + dynRelTable[i].r_addend;
          });
        } else if (relType == ElfRelocSection::R_X86_64_COPY) {
          const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
          const char* symName = elfInfo.getDynSymName(dynSym.st_name);
          getRelocateInfo(symName, true, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
            auto src = (void*)(relocateInfo.base() + relocateInfo.value() + dynRelTable[i].r_addend);
            auto dest = (void*)GLOBAL_REL_ADDR(dynRelTable[i].r_offset, elfInfo.getBase());
            memcpy(dest, src, dynSym.st_size);
          });
        } else if (relType == ElfRelocSection::R_X86_64_GLOB_DAT || relType == ElfRelocSection::R_X86_64_64) {
          const auto dynSym = dynSymTable[ELF64_R_SYM(dynRelTable[i].r_info)];
          const char* symName = elfInfo.getDynSymName(dynSym.st_name);
          auto rel_offset = (uint64_t*)GLOBAL_REL_ADDR(dynRelTable[i].r_offset, elfInfo.getBase());
          getRelocateInfo(symName, true, ELF64_ST_BIND(dynSym.st_info)).ifPresent([&](IRelocateInfo& relocateInfo) {
            //relocate only if the global symbol is from a different shared library
            if (relocateInfo.id() != dllEntry.second.id()) {
              *rel_offset = GLOBAL_REL_ADDR(relocateInfo.value() + dynRelTable[i].r_addend, relocateInfo.base());
            }
          });
        }
      }
    });

    elfInfo.loadInitFini(init_fini_list[init_fini_index]);
    ++init_fini_index;
  }
}