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
#include <mutex.h>
#include <exception.h>
#include <stdlib.h>

void MemManager::PageFaultHandler() {
	uint64_t faultyAddress ;
  __asm__ __volatile__("movq %%cr2, %0" : "=rm"(faultyAddress) : ) ;

	if (IS_KERNEL()) {
    printf("\n Page Fault in Kernel! FIX THIS !!! @ %lx", faultyAddress);
    while(1);
  }
	if(!KC::MKernelService().RequestPageFault(faultyAddress)) {
		ProcessManager_Exit();
	}
}

MemManager::MemManager() : RAM_SIZE(MultiBoot::Instance().GetRamSize()) {
  KC::MConsole().Message("\n MemManager Init\n", ' ');
  if(BuildRawPageMap()) {
    if(BuildPageTable()) {
      if (BuildPagePoolMap()) {
        if (MarkACPIInfoRegionAsAllocated()) {
          InitTaskState64();
          Mem_FlushTLB();
          DMM_InitAUTForKernel();
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

class TaskState64 {
public:
  uint32_t _reserved1;

  uint64_t _rsp0;
  uint64_t _rsp1;
  uint64_t _rsp2;

  uint64_t _reserved2;

  uint64_t _ist1;
  uint64_t _ist2;
  uint64_t _ist3;
  uint64_t _ist4;
  uint64_t _ist5;
  uint64_t _ist6;
  uint64_t _ist7;

  uint64_t _reserved3;
  uint16_t _reserved4;
  uint16_t _ioMapBase;
} PACKED;

void MemManager::InitTaskState64() {
  TaskState64* taskState64 = (TaskState64*)(TSS_BASE_ADDR);
  memset(taskState64, 0, sizeof(TaskState64));
  taskState64->_ioMapBase = 103;
  taskState64->_rsp0 = MEM_KERNEL_RING0_STACK_TOP;
  taskState64->_ist1 = MEM_KERNEL_IST1_TIMER_STACK_TOP;
  taskState64->_ist2 = MEM_KERNEL_IST2_PAGE_FAULT_STACK_TOP;
  taskState64->_ist3 = MEM_KERNEL_IST3_COMMON_STACK_TOP;

  __asm__ __volatile__("mov %0, %%ax;"
                       "ltr %%ax;" : : "m"(SYS_TSS_SELECTOR) :);
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
  _kernelPagePoolStartPage = MEM_KERNEL_PAGE_POOL_START / PAGE_SIZE;

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
  _noOfPages = RAM_SIZE / PAGE_SIZE;

  const auto noOfPTTableEntries = _noOfPages;
	if (noOfPTTableEntries > MEM_PT_SIZE) {
    KC::MConsole().Message("\n PT table size insufficient\n", 'A') ;
    return false ;
	}

  const auto noOfInitPages = I_DIVIDE_AND_CEIL(MEM_INIT_PAGE_MAP_SIZE, PAGE_SIZE);
	for (uint32_t i = noOfInitPages; i < noOfPTTableEntries; ++i) {
	  MEM_PT_TABLE[i] = (i * PAGE_SIZE) | 0x3;
	}

  for (uint32_t i = noOfPTTableEntries; i < MEM_PT_SIZE; ++i) {
    MEM_PT_TABLE[i] = 0x0;
  }

  const auto noOfPTs = I_DIVIDE_AND_CEIL(noOfPTTableEntries, ENTRIES_PER_PAGE_TABLE);
  if (noOfPTs > MEM_PD_SIZE) {
    KC::MConsole().Message("\n PD table size insufficient\n", 'A') ;
    return false ;
  }

  const auto noOfInitPTs = I_DIVIDE_AND_CEIL(noOfInitPages, ENTRIES_PER_PAGE_TABLE);
  for (uint32_t i = noOfInitPTs; i < noOfPTs; ++i) {
    MEM_PD_TABLE[i] = ((uint64_t)MEM_PT_TABLE + i * PAGE_SIZE) | 0x3;
  }

  for (uint32_t i = noOfPTs; i < MEM_PD_SIZE; ++i) {
    MEM_PD_TABLE[i] = 0x0;
  }

  const auto noOfPDs = I_DIVIDE_AND_CEIL(noOfPTs, ENTRIES_PER_PAGE_TABLE);
  if (noOfPDs > MEM_PDP_SIZE) {
    KC::MConsole().Message("\n PDP table size insufficient\n", 'A') ;
    return false ;
  }

  const auto noOfInitPDs = I_DIVIDE_AND_CEIL(noOfInitPTs, ENTRIES_PER_PAGE_TABLE);
  for (uint32_t i = noOfInitPDs; i < noOfPDs; ++i) {
    MEM_PDP_TABLE[i] = ((uint64_t)MEM_PD_TABLE + i * PAGE_SIZE) | 0x3;
  }

  for (uint32_t i = noOfPDs; i < MEM_PDP_SIZE; ++i) {
    MEM_PDP_TABLE[i] = 0x0;
  }

  const auto noOfPDPs = I_DIVIDE_AND_CEIL(noOfPDs, ENTRIES_PER_PAGE_TABLE);
  if (noOfPDPs > MEM_PML4_SIZE) {
    KC::MConsole().Message("\n PML4 table size insufficient\n", 'A') ;
    return false ;
  }

  const auto noOfInitPDPs = I_DIVIDE_AND_CEIL(noOfInitPDs, ENTRIES_PER_PAGE_TABLE);
  for (uint32_t i = noOfInitPDPs; i < noOfPDPs; ++i) {
    MEM_PML4_TABLE[i] = ((uint64_t)MEM_PDP_TABLE + i * PAGE_SIZE) | 0x3;
  }

  for (uint32_t i = noOfPDPs; i < MEM_PML4_SIZE; ++i) {
    MEM_PML4_TABLE[i] = 0x0;
  }

  /***** Initialize Kernel Processes Stack Pages Table Entries *****/
	for (bool& a : _allocMapForKernelProcessStackBlock) {
    a = false;
  }

	return true ;
}

void MemManager::KernelPageTableMmap(const uint64_t vAddr, const uint64_t pAddr, const uint32_t pageFlag) {
  auto pdpTable = PAGE_TABLE(MEM_PML4_TABLE, PML4_INDEX(vAddr));
  auto pdTable = PAGE_TABLE(pdpTable, PDP_INDEX(vAddr));
  auto ptTable = PAGE_TABLE(pdTable, PD_INDEX(vAddr));
  // This page is a Read Only area for user process. 0x3 => 011 => Supervisor, Read/Write, Present Bit
  ptTable[PT_INDEX(vAddr)] = (pAddr & ~0xFFF) | pageFlag;
}

uint64_t* MemManager::GetPTTable(uint64_t* pml4Table, uintptr_t virtualAddress) {
  auto pml4Index = PML4_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(pml4Table, pml4Index)) {
    auto pdpPage = AllocatePhysicalPage();
    pml4Table[pml4Index] = (pdpPage * PAGE_SIZE) | 0x7;
  }

  auto pdpTable = PAGE_TABLE(pml4Table, pml4Index);
  auto pdpIndex = PDP_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(pdpTable, pdpIndex)) {
    auto pdPage = AllocatePhysicalPage();
    pdpTable[pdpIndex] = (pdPage * PAGE_SIZE) | 0x7;
  }

  auto pdTable = PAGE_TABLE(pdpTable, pdpIndex);
  return GetPTTableFromPD(pdTable, virtualAddress);
}

uint64_t* MemManager::GetPTTableFromPD(uint64_t* pdTable, uintptr_t virtualAddress) {
  auto pdIndex = PD_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(pdTable, pdIndex)) {
    auto ptPage = AllocatePhysicalPage();
    pdTable[pdIndex] = (ptPage * PAGE_SIZE) | 0x7;
  }
  return PAGE_TABLE(pdTable, pdIndex);
}

void MemManager::AllocateAddressSpace(uint64_t* pml4Table, uint32_t pageConfig, uintptr_t virtualAddress, uintptr_t size) {
  for(const auto maxVirtualAddress = virtualAddress + size; virtualAddress < maxVirtualAddress; virtualAddress += PAGE_SIZE) {
    auto ptTable = GetPTTable(pml4Table, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
      ptTable[ptIndex] = AllocatePhysicalPage() * PAGE_SIZE | pageConfig;
    }
  }
}

void MemManager::AllocatePDAddressSpace(uint64_t* pdTable, uint32_t pageConfig, uintptr_t virtualAddress, uintptr_t size) {
  for(const auto maxVirtualAddress = virtualAddress + size; virtualAddress < maxVirtualAddress; virtualAddress += PAGE_SIZE) {
    auto ptTable = GetPTTableFromPD(pdTable, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
      ptTable[ptIndex] = AllocatePhysicalPage() * PAGE_SIZE | pageConfig;
    }
  }
}

void MemManager::DeallocateAddressSpace(uint64_t* pml4Table, uintptr_t virtualAddress, uintptr_t size) {
  const auto maxVirtualAddress = virtualAddress + size;
  for(; virtualAddress < maxVirtualAddress; virtualAddress += PAGE_SIZE) {
    auto ptTable = GetPTTable(pml4Table, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    if (PAGE_IS_PRESENT(ptTable, ptIndex)) {
      DeAllocatePhysicalPage(PAGE_ADDRESS(ptTable, ptIndex));
    }
  }
}

void MemManager::DeallocateAddressSpace(uint64_t* pml4Table) {
  for(int pml4Index = 0; pml4Index < ENTRIES_PER_PAGE_TABLE; ++pml4Index) {
    if (PAGE_IS_PRESENT(pml4Table, pml4Index)) {
      auto pdpTable = PAGE_TABLE(pml4Table, pml4Index);
      for(int pdpIndex = 0; pdpIndex < ENTRIES_PER_PAGE_TABLE; ++pdpIndex) {
        if (PAGE_IS_PRESENT(pdpTable, pdpIndex)) {
          auto pdTable = PAGE_TABLE(pdpTable, pdpIndex);
          DeallocatePDAddressSpace(pdTable);
        }
      }
      DeAllocatePhysicalPage(PAGE_ADDRESS(pml4Table, pml4Index));
    }
  }
  DeAllocatePhysicalPage((uint64_t)pml4Table / PAGE_SIZE);
}

void MemManager::DeallocatePDAddressSpace(uint64_t* pdTable) {
  for(int pdIndex = 0; pdIndex < ENTRIES_PER_PAGE_TABLE; ++pdIndex) {
    if (PAGE_IS_PRESENT(pdTable, pdIndex)) {
      auto ptTable = PAGE_TABLE(pdTable, pdIndex);
      for(int ptIndex = 0; ptIndex < ENTRIES_PER_PAGE_TABLE; ++ptIndex) {
        if (PAGE_IS_PRESENT(ptTable, ptIndex)) {
          DeAllocatePhysicalPage(PAGE_ADDRESS(ptTable, ptIndex));
        }
      }
      DeAllocatePhysicalPage(PAGE_ADDRESS(pdTable, pdIndex));
    }
  }
  DeAllocatePhysicalPage((uint64_t)pdTable / PAGE_SIZE);
}

void MemManager::MapAddressSpace(uint64_t* pml4Table,
                                 uint32_t pageConfig,
                                 uintptr_t virtualAddress,
                                 uintptr_t realAddress,
                                 uintptr_t size) {
  const auto maxVirtualAddress = virtualAddress + size;
  for(; virtualAddress < maxVirtualAddress; virtualAddress += PAGE_SIZE, realAddress += PAGE_SIZE) {
    auto ptTable = GetPTTable(pml4Table, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    ptTable[ptIndex] = realAddress | pageConfig;
  }
}

void MemManager::UnMapAddressSpace(uint64_t* pml4Table, uintptr_t virtualAddress, uintptr_t size) {
  const auto maxVirtualAddress = virtualAddress + size;
  for(; virtualAddress < maxVirtualAddress; virtualAddress += PAGE_SIZE) {
    auto ptTable = GetPTTable(pml4Table, virtualAddress);
    auto ptIndex = PT_INDEX(virtualAddress);
    if (PAGE_IS_PRESENT(ptTable, ptIndex)) {
      ptTable[ptIndex] = 0;
    }
  }
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
          uint64_t pageNumber = (pageMapPosition * 64) + pageOffset;
          memset((void*)(pageNumber * PAGE_SIZE), 0, PAGE_SIZE);
          return pageNumber;
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

extern __volatile__ uint64_t SYS_CALL_ID;

ReturnCode MemManager::AllocatePage(int iProcessID, uintptr_t faultyAddress) {
  upan::mutex_guard g(ProcessManager::Instance().GetSchedulableProcess(iProcessID).value().pageAllocMutex().value());

  const auto virtualPageNo = faultyAddress / PAGE_SIZE;
  if (ProcessManager::Instance().IsKernelProcess(iProcessID)) {
    printf("\n Page Fault in Kernel! FIX THIS !!!");
    printf("\n Page Fault Address/Page: %llx / %u", faultyAddress, virtualPageNo);
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
    printf("\n Sys Call Id: %lu", SYS_CALL_ID);
    printf("\n PID: %d, DMM Flag: %d", iProcessID, ProcessManager::Instance().IsDMMOn(iProcessID));
    return Failure;
  }

  auto pml4Table = (uint64_t *) ProcessManager::Instance().GetSchedulableProcess(iProcessID).value().pml4Table();
  auto ptTable = GetPTTable(pml4Table, faultyAddress);
  const auto ptIndex = PT_INDEX(faultyAddress);
  auto address = ptTable[ptIndex];

  if ((address & 0x1) == 0) {
    auto page = AllocatePhysicalPage();
    ptTable[ptIndex] = (page * PAGE_SIZE) | 0x7;
  } else if ((address & 0x7) == 0x7) {
    // we are good - page is already allocated - possibly because of a page fault on same address/page area from another thread.
  } else {
    /* Crash the Process..... With SegFault Or OutOfMemeory Error*/
    printf("\n Segmentation/Permission Fault @ Address: 0x%lx", faultyAddress);
    return Failure;
  }
  return Success;
}

uintptr_t MemManager::GetFlatAddress(uint64_t* pml4Table, uintptr_t virtualAddress) {
  const auto pml4Index = PML4_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(pml4Table, pml4Index)) {
    return NULL;
  }

  auto pdpTable = PAGE_TABLE(pml4Table, pml4Index);
  const auto pdpIndex = PDP_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(pdpTable, pdpIndex)) {
    return NULL;
  }

  auto pdTable = PAGE_TABLE(pdpTable, pdpIndex);
  return GetFlatAddressFromPD(pdTable, virtualAddress);
}

uintptr_t MemManager::GetFlatAddressFromPD(uint64_t* pdTable, uintptr_t virtualAddress) {
  const auto pdIndex = PD_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(pdTable, pdIndex)) {
    return NULL;
  }

  auto ptTable = PAGE_TABLE(pdTable, pdIndex);
  const auto ptIndex = PT_INDEX(virtualAddress);
  if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
    return NULL;
  }

  return PAGE_ADDRESS(ptTable, ptIndex) + PAGE_INDEX(virtualAddress);
}

uint64_t MemManager::GetCeilAlignedAddress(uint64_t uiAddress, unsigned uiAlign) {
  while(true) {
    if((uiAddress % uiAlign) == 0)
      return uiAddress;
    uiAddress++;
  }
  return 0;
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

