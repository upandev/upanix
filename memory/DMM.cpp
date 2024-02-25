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
#include <DMM.h>
#include <MemManager.h>
#include <UpanixMain.h>
#include <stdio.h>
#include <exception.h>
#include <ProcessManager.h>
#include <Bit.h>

static AllocationUnitTracker* DMM_kernelAUTAddress = nullptr;

/*************** static (private) functions ********************/

static uint32_t DMM_GetByteStuffForAlign(uint64_t uiAddress, uint32_t uiAlignNumber) {
  uint32_t uiByteStuffForAlign = 0 ;
	if(uiAlignNumber != 0) {
    uint32_t uiAlignMod = uiAddress % uiAlignNumber ;
		if(uiAlignMod != 0)
			uiByteStuffForAlign = uiAlignNumber - uiAlignMod ;
	}
	return uiByteStuffForAlign ;
}

void DMM_CheckAlignNumber(uint32_t uiAlignNumber) {
  if(uiAlignNumber == 0)
    return;
  if(Bit::IsPowerOfTwo(uiAlignNumber))
    return;
  throw upan::exception(XLOC, "%u is not 2^ power aligned address", uiAlignNumber);
}

static uint64_t calculateCheckSum(AllocationUnitTracker& aut) {
  return uint64_t(aut.allocatedAddress) ^ aut.size ^ aut.returnAddress ^ uint64_t(aut.nextAUTAddress);
}

static void updateCheckSum(AllocationUnitTracker& aut) {
  aut.checkSum = calculateCheckSum(aut);
}

/***************************************************************/

// old allocation algorithm which was maintaining a list of allocated chunks was 40 times slower
// than the current algorithm which maintains a list of free chunks
uintptr_t DMM_Allocate(Process* processAddressSpace, uint32_t sizeInBytes, uint32_t alignNumber) {
  upan::mutex_guard g(processAddressSpace->heapMutex().value());
  DMM_CheckAlignNumber(alignNumber);
  processAddressSpace->setDmmFlag(true);
  auto heapStartAddress = (AllocationUnitTracker*)PROCESS_HEAP_START_ADDRESS;

  AllocationUnitTracker *aut, *prevAut ;
  // TODO: Limit Check on Allocated Memory i.e, Allocation should be limited to either
  // RAM Size OR 4GB if virtual memory management is implemented

  // Dedicated Head Node. This will avoid Back Loop at Head
  if(processAddressSpace->getAUTAddress() == nullptr) {
    aut = heapStartAddress;

    aut->allocatedAddress = heapStartAddress ;
    aut->size = PROCESS_HEAP_SIZE ;
    aut->returnAddress = NULL;
    aut->nextAUTAddress = nullptr;
    processAddressSpace->setAUTAddress(heapStartAddress);
  }

  aut = processAddressSpace->getAUTAddress();
  prevAut = nullptr;
  while(aut != nullptr) {
    auto address = (uintptr_t)aut->allocatedAddress;
    uint64_t maxSize = aut->size;
    auto nextAutAddress = aut->nextAUTAddress;
    uintptr_t byteStuffForAlign = DMM_GetByteStuffForAlign(address + sizeof(AllocationUnitTracker), alignNumber);
    uint64_t size = sizeInBytes + sizeof(AllocationUnitTracker) + byteStuffForAlign;

    if(size <= maxSize) {
      auto allocAUT = (AllocationUnitTracker*)(address + byteStuffForAlign);
      allocAUT->allocatedAddress = (AllocationUnitTracker*)address;
      allocAUT->returnAddress = address + sizeof(AllocationUnitTracker) + byteStuffForAlign;
      allocAUT->size = size;
      allocAUT->byteStuffForAlign = byteStuffForAlign;

      uint64_t remaining = maxSize - size;
      if(remaining > (sizeof(AllocationUnitTracker) + 1)) {
        auto freeAUT = (AllocationUnitTracker*)(address + size);
        freeAUT->allocatedAddress = freeAUT;
        freeAUT->returnAddress = NULL;
        freeAUT->size = remaining;
        freeAUT->nextAUTAddress = nextAutAddress;

        if(prevAut == nullptr)
          processAddressSpace->setAUTAddress(freeAUT->allocatedAddress);
        else
          prevAut->nextAUTAddress = freeAUT->allocatedAddress;
      } else {
        allocAUT->size += remaining;
        if(prevAut == nullptr)
          processAddressSpace->setAUTAddress(nextAutAddress);
        else
          prevAut->nextAUTAddress = nextAutAddress;
      }

      //Make sure that all pages are allocated in the requested mem block
      uintptr_t addr = address + sizeof(AllocationUnitTracker) ;
      UNUSED __volatile__ int x;
      while(addr < (address + aut->size)) {
        x = ((char*)addr)[0] ; // A read would cause a page fault
        addr += PAGE_SIZE ;
      }
      x = ((char*)(address + aut->size - 1))[0] ;

      processAddressSpace->setDmmFlag(false);
      return PROCESS_VIRTUAL_ALLOCATED_ADDRESS(aut->returnAddress) ;
    }

    prevAut = aut;
    aut = nextAutAddress;
  }
  throw upan::exception(XLOC, "out of memory!");
}

static uintptr_t DMM_GetKernelHeapStartAddress() {
  return MEM_KERNEL_HEAP_START;
}

void DMM_InitAUTForKernel() {
  auto aut = (AllocationUnitTracker*)DMM_GetKernelHeapStartAddress();
  aut->allocatedAddress = aut;
  aut->size = MEM_KERNEL_HEAP_SIZE;
  aut->returnAddress = NULL;
  aut->nextAUTAddress = nullptr;
  updateCheckSum(*aut);
  DMM_kernelAUTAddress = aut;
}

uint32_t dmm_alloc_count = 0;
uintptr_t DMM_AllocateForKernel(unsigned sizeInBytes, unsigned alignNumber) {
  IrqGuard g;
  ++dmm_alloc_count;
	DMM_CheckAlignNumber(alignNumber);

  auto aut = DMM_kernelAUTAddress;
  AllocationUnitTracker* prevAut = nullptr;
  while(aut != nullptr) {
    auto address = (uintptr_t)aut->allocatedAddress;
    uint64_t maxSize = aut->size;
    auto nextAUTAddress = aut->nextAUTAddress;
    uintptr_t byteStuffForAlign = DMM_GetByteStuffForAlign(address + sizeof(AllocationUnitTracker), alignNumber);
    uint64_t size = sizeInBytes + sizeof(AllocationUnitTracker) + byteStuffForAlign;
    const uint64_t calcCheckSum = calculateCheckSum(*aut);
    if (calcCheckSum != aut->checkSum) {
      throw upan::exception(XLOC,"heap corrupted!");
    }
    if(size <= maxSize) {
      auto allocAUT = (AllocationUnitTracker*)(address + byteStuffForAlign);
      allocAUT->allocatedAddress = (AllocationUnitTracker*)address;
      allocAUT->returnAddress = address + sizeof(AllocationUnitTracker) + byteStuffForAlign;
      allocAUT->size = size;
      allocAUT->byteStuffForAlign = byteStuffForAlign;

      uint64_t remaining = maxSize - size;
      if(remaining > (sizeof(AllocationUnitTracker) + 1)) {
        auto freeAUT = (AllocationUnitTracker*)(address + size);
        freeAUT->allocatedAddress = freeAUT;
        freeAUT->returnAddress = NULL;
        freeAUT->size = remaining;
        freeAUT->nextAUTAddress = nextAUTAddress;

        if(prevAut == nullptr)
          DMM_kernelAUTAddress = freeAUT->allocatedAddress;
        else
          prevAut->nextAUTAddress = freeAUT->allocatedAddress;
        updateCheckSum(*freeAUT);
      }
      else
      {
        allocAUT->size += remaining;
        if(prevAut == nullptr)
          DMM_kernelAUTAddress = nextAUTAddress;
        else
          prevAut->nextAUTAddress = nextAUTAddress;
      }
      if (prevAut != nullptr) {
        updateCheckSum(*prevAut);
      }
      updateCheckSum(*allocAUT);
      return allocAUT->returnAddress;
    }
    prevAut = aut;
    aut = nextAUTAddress;
  }
  throw upan::exception(XLOC, "out of memory!");
}

byte DMM_DeAllocate(Process* processAddressSpace, uintptr_t address) {
  upan::mutex_guard g(processAddressSpace->heapMutex().value());
  // do this before converting the address to real address (by adding PROCESS_BASE)
  if(address == NULL)
    return DMM_SUCCESS ;

  address = PROCESS_REAL_ALLOCATED_ADDRESS(address) ;
  uint64_t heapStartAddress = PROCESS_HEAP_START_ADDRESS;

  if(address <= heapStartAddress)
    return DMM_BAD_DEALLOC ;

  auto freeAUT = (AllocationUnitTracker*)(address - sizeof(AllocationUnitTracker));
  uintptr_t allocatedAddress = address - sizeof(AllocationUnitTracker) - freeAUT->byteStuffForAlign;
  uintptr_t size = freeAUT->size;
  freeAUT = (AllocationUnitTracker*)allocatedAddress;
  freeAUT->allocatedAddress = freeAUT;
  freeAUT->returnAddress = NULL;
  freeAUT->size = size;
  freeAUT->nextAUTAddress = processAddressSpace->getAUTAddress();
  processAddressSpace->setAUTAddress(freeAUT);
  return DMM_SUCCESS;
}

bool DMM_GetAllocSize(uintptr_t address, size_t* size) {
  address = PROCESS_REAL_ALLOCATED_ADDRESS(address);
  uintptr_t heapStartAddress = PROCESS_HEAP_START_ADDRESS;

  if (address == NULL || address < sizeof(AllocationUnitTracker) || address == heapStartAddress) {
    *size = 0;
    return false;
  }

  auto aut = (AllocationUnitTracker *) (address - sizeof(AllocationUnitTracker));
  *size = aut->size;
  return true;
}

bool DMM_GetAllocSizeForKernel(uintptr_t uiAddress, size_t* size) {
  IrqGuard g;
  uintptr_t uiHeapStartAddress = DMM_GetKernelHeapStartAddress();
  if (uiAddress == NULL || uiAddress < sizeof(AllocationUnitTracker) || uiAddress == uiHeapStartAddress) {
    *size = 0;
    return false;
  }

  auto aut = (AllocationUnitTracker *) (uiAddress - sizeof(AllocationUnitTracker));
  *size = aut->size;
  return true;
}

bool DMM_DeAllocateForKernel(uintptr_t address) {
  IrqGuard g;
	if(address == NULL)
		return true;

	if(address <= DMM_GetKernelHeapStartAddress())
		return false;

  auto freeAUT = (AllocationUnitTracker*)(address - sizeof(AllocationUnitTracker));
  uintptr_t allocatedAddress = address - sizeof(AllocationUnitTracker) - freeAUT->byteStuffForAlign;
  uint64_t size = freeAUT->size;
  const uint64_t calcCheckSum = calculateCheckSum(*freeAUT);
  if (calcCheckSum != freeAUT->checkSum) {
    throw upan::exception(XLOC, "bad address dealloc %x", address);
  }
  if (freeAUT->returnAddress == NULL) {
    throw upan::exception(XLOC, "double delete %x", address);
  }
  freeAUT = (AllocationUnitTracker*)allocatedAddress;
  freeAUT->allocatedAddress = freeAUT;
  freeAUT->returnAddress = NULL;
  freeAUT->size = size;
  freeAUT->nextAUTAddress = DMM_kernelAUTAddress;
  updateCheckSum(*freeAUT);
  DMM_kernelAUTAddress = freeAUT;
  return true;
}
	
void DMM_DeAllocatePhysicalPages(Process* processAddressSpace) {
	auto pml4Table = (uint64_t*) processAddressSpace->pml4Table();
  auto pdpTable = (uint64_t*)(pml4Table[0] & PAGE_MASK);

	for(uint64_t address = PROCESS_HEAP_START_ADDRESS; address < (PROCESS_HEAP_START_ADDRESS + PROCESS_HEAP_SIZE); address += PAGE_SIZE) {
    const auto pdpIndex = PDP_INDEX(address);

    if (PAGE_IS_PRESENT(pdpTable, pdpIndex)) {
      auto pdTable = PAGE_TABLE(pdpTable, pdpIndex);

      const auto pdIndex = PD_INDEX(address);
      if (PAGE_IS_PRESENT(pdTable, pdIndex)) {
        auto ptTable = PAGE_TABLE(pdTable, pdIndex);

        const auto ptIndex = PT_INDEX(address);
        if (PAGE_IS_PRESENT(ptTable, ptIndex)) {
          auto realAddress = PAGE_ADDRESS(ptTable, ptIndex);
          MemManager::Instance().DeAllocatePhysicalPage(realAddress / PAGE_SIZE);
          continue;
        }
      }
    }
    break;
	}
}

unsigned DMM_KernelHeapAllocSize() {
	uint64_t size = 0 ;
	for(auto aut = DMM_kernelAUTAddress; aut != nullptr; aut = aut->nextAUTAddress)	{
    size += aut->size;
	}
	return size ;
}