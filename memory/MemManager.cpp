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
#include <MemManager.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <KernelService.h>
#include <MultiBoot.h>
#include <MemUtil.h>
#include <AsmUtil.h>
#include <IDT.h>
#include <mutex.h>
#include <exception.h>
#include <GraphicsVideo.h>
#include <RootGUIConsole.h>

extern "C" { 
	uint64_t MEM_PML4 ;
}

void MemManager::PageFaultHandlerTaskGate(uint64_t errorCode) {
	__volatile__ uint64_t faultyAddress ;
	__asm__ __volatile__("mov %%cr2, %0" : "=r"(faultyAddress) : ) ;

	if (IS_KERNEL()) {
    printf("\n Page Fault in Kernel! FIX THIS !!! @ %ul", faultyAddress);
    while(1);
  }
	if(!KC::MKernelService().RequestPageFault(faultyAddress)) {
		ProcessManager_EXIT() ;
	}
}

MemManager::MemManager() : _kernelAUTAddress(NULL), RAM_SIZE(MultiBoot::Instance().GetRamSize()) {
  KC::MConsole().Message("\n MemManager Init\n", ' ');
  if(BuildRawPageMap()) {
    if(BuildPageTable()) {
      if (BuildPagePoolMap()) {
        if (MarkACPIInfoRegionAsAllocated()) {
          Mem_FlushTLB();
          KC::MConsole().LoadMessage("Memory Manager Initialization", Success);
          return;
        }
      }
    }
  }

  KC::MConsole().Message("\n *********** KERNEL PANIC ************ \n", '$') ;
  while(1) ;
}

void MemManager::PrintInitStatus() const {
  printf("\n\tRAM SIZE = %ul", RAM_SIZE) ;
  printf("\n\tNo. of Pages = %d", _noOfPages) ;
  printf("\n\tNo. of Resv Pages = %d\n", _kernelReservedPages) ;
}

bool MemManager::MarkACPIInfoRegionAsAllocated() {
  auto acpiMmap = MultiBoot::Instance().GetACPIInfoMemMap();
  if (!acpiMmap) {
    return true;
  }

  const uint32_t noOfPages = ((acpiMmap->length + PAGE_SIZE) / PAGE_SIZE) - 1;
  uint32_t addr = acpiMmap->addr;

  ReturnCode markPageRetCode = Success;
  for(int i = 0; i < noOfPages; ++i) {
    markPageRetCode = MarkPageAsAllocated(addr / PAGE_SIZE, markPageRetCode);
    if (markPageRetCode != Success) {
      return false;
    }
    addr += PAGE_SIZE;
  }

  return true;
}

void MemManager::InitPage(uint64_t pageNum) {
  memset((void*)(pageNum * PAGE_SIZE), 0, PAGE_SIZE);
}

bool MemManager::BuildRawPageMap() {
  _pageMap = (uintptr_t*)MEM_PAGE_MAP_START;
  _pageMapSize = (((RAM_SIZE / PAGE_SIZE) / 8) / 8) ;
  _kernelReservedMapSize = (((MEM_KERNEL_RESV_SIZE / PAGE_SIZE) / 8) / 8);
  _kernelReservedPages = MEM_KERNEL_RESV_SIZE / PAGE_SIZE;
  _kernelHeapMapSize = (MEM_KERNEL_HEAP_SIZE / PAGE_SIZE) / 8 / 8;

  if((_pageMapSize * 8) > (MEM_PAGE_MAP_END - MEM_PAGE_MAP_START)) {
    KC::MConsole().Message("\n Mem Page Map Size InSufficient\n", 'A') ;
    return false ;
  }

  for(uint32_t i = 0; i < _pageMapSize; ++i) {
    _pageMap[i] &= 0x0;
  }

  for(uint32_t i = 0; i < _kernelReservedMapSize; ++i) {
    _pageMap[i] |= UINT64_MAX;
  }

  return true ;
}	

bool MemManager::BuildPagePoolMap() {
  _kernelPagePoolMap = (uint64_t*)MEM_KERNEL_PAGE_POOL_MAP_START;
  _kernelPagePoolMapSize = MEM_KERNEL_PAGE_POOL_SIZE / PAGE_SIZE / 8 / sizeof(uint64_t);
  _kernelPagePoolStartPage = (MEM_KERNEL_HEAP_START + MEM_KERNEL_HEAP_SIZE) / PAGE_SIZE;

  if((_kernelPagePoolMapSize * sizeof(uint64_t)) > (MEM_KERNEL_PAGE_POOL_MAP_END - MEM_KERNEL_PAGE_POOL_MAP_START)) {
    KC::MConsole().Message("\n Mem Page Pool Map Size InSufficient\n", 'A') ;
    return false ;
  }

  for(uint32_t i = 0; i < _kernelPagePoolMapSize; ++i) {
    _kernelPagePoolMap[i] &= 0x0;
  }

  return true;
}

bool MemManager::BuildPageTable() {
	MEM_PML4 = (uint64_t)MEM_PML4_TABLE;

  _noOfPages = RAM_SIZE / PAGE_SIZE;

  const auto noOfPTTableEntries = _noOfPages;
	if (noOfPTTableEntries > MEM_PT_SIZE) {
    KC::MConsole().Message("\n PT table size insufficient\n", 'A') ;
    return false ;
	}

  const auto noOfInitPages = MEM_INIT_PAGE_MAP_SIZE / PAGE_SIZE;
	for (uint32_t i = noOfInitPages; i < noOfPTTableEntries; ++i) {
	  MEM_PT_TABLE[i] = (i * PAGE_SIZE) | 0x3;
	}

	const auto noOfPTs = noOfPTTableEntries / ENTRIES_PER_PAGE_TABLE;
  if (noOfPTs > MEM_PD_SIZE) {
    KC::MConsole().Message("\n PD table size insufficient\n", 'A') ;
    return false ;
  }

  const auto noOfInitPTs = noOfInitPages / ENTRIES_PER_PAGE_TABLE;
  for (uint32_t i = noOfInitPTs; i < noOfPTs; ++i) {
    MEM_PD_TABLE[i] = ((uint64_t)MEM_PT_TABLE + i * PAGE_SIZE) | 0x3;
  }

  const auto noOfPDs = noOfPTs / ENTRIES_PER_PAGE_TABLE;
  if (noOfPDs > MEM_PDP_SIZE) {
    KC::MConsole().Message("\n PDP table size insufficient\n", 'A') ;
    return false ;
  }

  const auto noOfInitPDs = noOfInitPTs / ENTRIES_PER_PAGE_TABLE;
  for (uint32_t i = noOfInitPDs; i < noOfPDs; ++i) {
    MEM_PDP_TABLE[i] = ((uint64_t)MEM_PD_TABLE + i * PAGE_SIZE) | 0x3;
  }

  const auto noOfPDPs = noOfPDs / ENTRIES_PER_PAGE_TABLE;
  if (noOfPDPs > MEM_PML4_SIZE) {
    KC::MConsole().Message("\n PML4 table size insufficient\n", 'A') ;
    return false ;
  }

  const auto noOfInitPDPs = noOfInitPDs / ENTRIES_PER_PAGE_TABLE;
  for (uint32_t i = noOfInitPDPs; i < noOfPDPs; ++i) {
    MEM_PML4_TABLE[i] = ((uint64_t)MEM_PDP_TABLE + i * PAGE_SIZE) | 0x3;
  }

  for (uint32_t i = noOfPDPs; i < 512; ++i) {
    MEM_PML4_TABLE[i] = 0x0;
  }

  /***** Initialize Kernel Processes Stack Pages Table Entries *****/
	for (bool& a : _allocMapForKernelProcessStackBlock) {
    a = false;
  }

	return true ;
}

void MemManager::Mmap(const uint64_t vAddr, const uint64_t pAddr, const uint32_t pageFlag) {
  auto pdpEntry = (uint64_t *) (MEM_PML4_TABLE[PML4_INDEX(vAddr)] & ~0xFFF);
  auto pdEntry = (uint64_t *) (pdpEntry[PDP_INDEX(vAddr)] & ~0xFFF);
  auto ptEntry = (uint64_t *) (pdEntry[PD_INDEX(vAddr)] & ~0xFFF);
  // This page is a Read Only area for user process. 0x3 => 011 => Supervisor, Read/Write, Present Bit
  ptEntry[PT_INDEX(vAddr)] = (pAddr & ~0xFFF) | pageFlag;
}

int MemManager::AllocateKernelStack() {
  ProcessSwitchLock pLock;
  for (int i = 0; i < NO_OF_KERNEL_STACK_BLOCKS; i++) {
    if (!_allocMapForKernelProcessStackBlock[i]) {
      _allocMapForKernelProcessStackBlock[i] = true;
      return i;
    }
  }
  throw upan::exception(XLOC, "No free stack blocks available for kernel process");
}

void MemManager::DeAllocateKernelStack(int stackBlockId) {
  ProcessSwitchLock pLock;

  if(stackBlockId < 0 || stackBlockId >= NO_OF_KERNEL_STACK_BLOCKS)
    return;

  _allocMapForKernelProcessStackBlock[stackBlockId] = false ;
}

ReturnCode MemManager::MarkPageAsAllocated(unsigned uiPageNumber, ReturnCode prevRetCode) {
  ProcessSwitchLock pLock;
  unsigned uiPageMapIndex = uiPageNumber / 64;
  //MEM IO addresses may fall beyond actual ram size - no need to mark such pages as allocated
  if (uiPageMapIndex >= _pageMapSize)
    return Success;
  unsigned uiPageBitIndex = uiPageNumber % 64;
  unsigned uiCurVal = (_pageMap[uiPageMapIndex] >> uiPageBitIndex) & 0x1;
  if (uiCurVal) {
    if (prevRetCode == Success) {
      printf("\n Error: Page: %u is already marked as allocated", uiPageNumber);
    }
    return Failure;
  }
  _pageMap[uiPageMapIndex] |= (0x1 << uiPageBitIndex);
  return Success;
}

uint64_t MemManager::AllocatePhysicalPage() {
	ProcessSwitchLock lock;
	for(auto pageMapPosition = _kernelReservedMapSize; pageMapPosition < _pageMapSize; ++pageMapPosition) {
		if((_pageMap[pageMapPosition] & UINT64_MAX) != UINT64_MAX) {
      auto pageMapEntry = _pageMap[pageMapPosition] ;
			for(auto pageOffset = 0; pageOffset < 64; ++pageOffset) {
				if((pageMapEntry & 0x1) == 0x0) {
          _pageMap[pageMapPosition] |= (0x1 << pageOffset) ;
          return (pageMapPosition * 64) + pageOffset;
				}
        pageMapEntry >>= 1 ;
			}
		}
	}
  throw upan::exception(XLOC, "Out of memory pages!");
}

void MemManager::DeAllocatePhysicalPage(uint64_t pageNumber)  {
  ProcessSwitchLock pLock;
	const auto pageMapPosition = pageNumber / 64;
	const auto pageOffset = pageNumber % 64;
  _pageMap[pageMapPosition] = _pageMap[pageMapPosition] & ~(0x1 << pageOffset) ;
}

unsigned MemManager::AllocatePageForKernel() {
  ProcessSwitchLock lock;
  for(auto i = 0; i < _kernelPagePoolMapSize; ++i) {
    if((_kernelPagePoolMap[i] & UINT64_MAX) != UINT64_MAX) {
      auto pageMapEntry = _kernelPagePoolMap[i];
      for(auto pageOffset = 0; pageOffset < 64; ++pageOffset) {
        if((pageMapEntry & 0x1) == 0x0) {
          _kernelPagePoolMap[i] |= (0x1 << pageOffset);
          return (i * 64) + pageOffset + _kernelPagePoolStartPage;
        }
        pageMapEntry >>= 1;
      }
    }
  }
  throw upan::exception(XLOC, "Out of memory pages in kernel pool!");
}

void MemManager::DeAllocatePageForKernel(uint32_t pageNumber) {
  ProcessSwitchLock lock;
  pageNumber -= _kernelPagePoolStartPage;
  const auto pageMapPosition = pageNumber / 64;
  const auto pageOffset = pageNumber % 64;
  _kernelPagePoolMap[pageMapPosition] = _kernelPagePoolMap[pageMapPosition] & ~(0x1 << pageOffset) ;
}

extern __volatile__ int SYS_CALL_ID;

ReturnCode MemManager::AllocatePage(int iProcessID, uintptr_t faultyAddress) {
  upan::mutex_guard g(ProcessManager::Instance().GetSchedulableProcess(iProcessID).value().pageAllocMutex().value());

  const auto virtualPageNo = faultyAddress / PAGE_SIZE;

  if (ProcessManager::Instance().IsKernelProcess(iProcessID)) {
    printf("\n Page Fault in Kernel! FIX THIS !!!");
    printf("\n Page Fault Address/Page: %x / %u", faultyAddress, virtualPageNo);
    __asm__ __volatile__ ("HLT");
    while (true);
  }

  bool permittedAddressAccess = false;
  //This space is for process Stack - page fault here should be only while expanding stack and not for Heap (DMM is OFF)
  if (faultyAddress >= (PROCESS_STACK_TOP_ADDRESS - PROCESS_STACK_SIZE)
      && faultyAddress < PROCESS_STACK_TOP_ADDRESS
      && !ProcessManager::Instance().IsDMMOn(iProcessID)) {
    permittedAddressAccess = true;
  } //page fault in heap while allocating memory (DMM is ON)
  else if (faultyAddress >= PROCESS_HEAP_START_ADDRESS
           && faultyAddress < (PROCESS_HEAP_START_ADDRESS + PROCESS_HEAP_SIZE)
           && ProcessManager::Instance().IsDMMOn(iProcessID)) {
    permittedAddressAccess = true;
  }
  if (!permittedAddressAccess) {
    printf("\n Segmentation Fault @ Address: 0x%llx", faultyAddress);
    printf("\n Sys Call Id: %d", SYS_CALL_ID);
    printf("\n PID: %d, DMM Flag: %d", iProcessID, ProcessManager::Instance().IsDMMOn(iProcessID));
    return Failure;
  }

  uint64_t *pml4 = (uint64_t *) ProcessManager::Instance().GetSchedulableProcess(iProcessID).value().pdbr();
  const auto pml4Index = PML4_INDEX(faultyAddress);
  const auto pdpIndex = PDP_INDEX(faultyAddress);
  const auto pdIndex = PD_INDEX(faultyAddress);
  const auto ptIndex = PT_INDEX(faultyAddress);

  auto pdp = pml4[pml4Index];
  if ((pdp & 0x1) == 0) {
    auto pdpPage = AllocatePhysicalPage();
    InitPage(pdpPage);
    pml4[pml4Index] = (pdpPage * PAGE_SIZE) | 0x7;
  }

  auto pdpTable = (uint64_t*)(pml4[pml4Index] & PAGE_MASK);
  auto pd = pdpTable[pdpIndex];
  if ((pd & 0x1) == 0) {
    auto pdPage = AllocatePhysicalPage();
    InitPage(pdPage);
    pdpTable[pdpIndex] = (pdPage * PAGE_SIZE) | 0x7;
  }

  auto pdTable = (uint64_t*)(pdpTable[pdpIndex] & PAGE_MASK);
  auto pt = pdTable[pdIndex];
  if ((pt & 0x1) == 0) {
    auto ptPage = AllocatePhysicalPage();
    InitPage(ptPage);
    pdTable[pdIndex] = (ptPage * PAGE_SIZE) | 0x7;
  }

  auto ptTable = (uint64_t*)(pdTable[pdIndex] & PAGE_MASK);
  auto address = ptTable[ptIndex];

  if ((address & 0x1) == 0) {
    auto page = AllocatePhysicalPage();
    InitPage(page);
    ptTable[ptIndex] = (page * PAGE_SIZE) | 0x7;
  } else if ((address & 0x7) == 0x7) {
    // we are good - page is already allocated - possibly because of a page fault on same address/page area from another thread.
  } else {
    /* Crash the Process..... With SegFault Or OutOfMemeory Error*/
    printf("\n Segmentation/Permission Fault @ Address: 0x%;;x", faultyAddress);
    return Failure;
  }
  return Success;
}

uintptr_t MemManager::GetFlatAddress(uintptr_t virtualAddress) {
  uint64_t *pml4 = (uint64_t *) ProcessManager::Instance().GetCurrentPAS().pdbr();
  const auto pml4Index = PML4_INDEX(virtualAddress);
  const auto pdpIndex = PDP_INDEX(virtualAddress);
  const auto pdIndex = PD_INDEX(virtualAddress);
  const auto ptIndex = PT_INDEX(virtualAddress);

  auto pdp = pml4[pml4Index];
  if ((pdp & 0x1) == 0) {
    return NULL;
  }
  pdp &= PAGE_MASK;
  auto pd = ((uint64_t *) pdp)[pdpIndex];
  if ((pd & 0x1) == 0) {
    return NULL;
  }

  pd &= PAGE_MASK;
  auto pt = ((uint64_t *) pd)[pdIndex];
  if ((pt & 0x1) == 0) {
    return NULL;
  }

  pt &= PAGE_MASK;
  auto address = ((uint64_t *) pt)[ptIndex];
  if ((address & 0x1) == 0) {
    return NULL;
  }

  address &= PAGE_MASK;
  return address + PAGE_INDEX(virtualAddress);
}

void MemManager::DisplayNoOfFreePages() {
	uint32_t freePageCount = 0 ;
	for(auto pageMapPosition = _kernelReservedMapSize; pageMapPosition < _pageMapSize; ++pageMapPosition) {
		if((_pageMap[pageMapPosition] & UINT64_MAX) != UINT64_MAX) {
      auto pageMapEntry = _pageMap[pageMapPosition] ;
			for(auto pageOffset = 0; pageOffset < 64; ++pageOffset) {
				if((pageMapEntry & 0x1) == 0x0) {
          freePageCount++;
        }
        pageMapEntry >>= 1 ;
			}
		}
	}
	printf("\n Free Page Count = %d", freePageCount);
}

void MemManager::DisplayNoOfAllocPages() {
	uint32_t allocPageCount = 0 ;
	for(auto pageMapPosition = _kernelReservedMapSize + _kernelHeapMapSize; pageMapPosition < _pageMapSize; ++pageMapPosition) {
		if((_pageMap[pageMapPosition] & UINT64_MAX) != 0) {
      auto pageMapEntry = _pageMap[pageMapPosition] ;
			for(auto pageOffset = 0; pageOffset < 64; ++pageOffset) {
				if((pageMapEntry & 0x1) == 0x1) {
					++allocPageCount;
				}
        pageMapEntry >>= 1 ;
			}
		}
	}
	printf("\n Alloc Page Count = %u\n", allocPageCount) ;
}

