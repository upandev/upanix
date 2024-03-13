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
#include <Bit.h>

uint64_t AllocationUnitTracker::calculateCheckSum() const {
  return uint64_t(allocatedAddress) ^ size ^ returnAddress ^ uint64_t(nextAUTAddress);
}

void AllocationUnitTracker::updateCheckSum() {
  checkSum = calculateCheckSum();
}

DMM::DMM(uint64_t heapStartAddress, uint64_t heapMaxSize) :
  _heapStartAddress(heapStartAddress), _heapMaxSize(heapMaxSize), _rootAut(nullptr) {
}

uint32_t DMM::getByteStuffForAlign(uint64_t uiAddress, uint32_t uiAlignNumber) const {
  uint32_t uiByteStuffForAlign = 0 ;
  if(uiAlignNumber != 0) {
    uint32_t uiAlignMod = uiAddress % uiAlignNumber ;
    if(uiAlignMod != 0)
      uiByteStuffForAlign = uiAlignNumber - uiAlignMod ;
  }
  return uiByteStuffForAlign ;
}

void DMM::validateAlignParam(uint32_t alignment) const {
  if(alignment == 0)
    return;
  if(Bit::IsPowerOfTwo(alignment))
    return;
  throw upan::exception(XLOC, "%u is not 2^ power aligned address", alignment);
}

uintptr_t DMM::_allocate(uint32_t sizeInBytes, uint32_t alignment) {
  validateAlignParam(alignment);
  _dmmFlag = true;

  //Dedicated Head Node. This will avoid Back Loop at Head
  //lazy initialize because accessing heap address could cause page fault - so, access address after calling process is fully initialized
  if (_rootAut == nullptr) {
    _rootAut = (AllocationUnitTracker *) _heapStartAddress;
    _rootAut->allocatedAddress = _rootAut;
    _rootAut->size = _heapMaxSize;
    _rootAut->returnAddress = NULL;
    _rootAut->nextAUTAddress = nullptr;
    _rootAut->updateCheckSum();
  }

  auto aut = _rootAut;
  AllocationUnitTracker* prevAut = nullptr;
  while(aut != nullptr) {
    auto address = (uintptr_t)aut->allocatedAddress;
    const uint64_t maxSize = aut->size;
    auto nextAUTAddress = aut->nextAUTAddress;
    const uintptr_t byteStuffForAlign = getByteStuffForAlign(address + sizeof(AllocationUnitTracker), alignment);
    const uint64_t size = sizeInBytes + sizeof(AllocationUnitTracker) + byteStuffForAlign;
    const uint64_t calcCheckSum = aut->calculateCheckSum();
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
          _rootAut = freeAUT->allocatedAddress;
        else
          prevAut->nextAUTAddress = freeAUT->allocatedAddress;
        freeAUT->updateCheckSum();
      }
      else
      {
        allocAUT->size += remaining;
        if(prevAut == nullptr)
          _rootAut = nextAUTAddress;
        else
          prevAut->nextAUTAddress = nextAUTAddress;
      }
      if (prevAut != nullptr) {
        prevAut->updateCheckSum();
      }
      allocAUT->updateCheckSum();
      accessMem(allocAUT);
      _dmmFlag = false;
      return allocAUT->returnAddress;
    }
    prevAut = aut;
    aut = nextAUTAddress;
  }
  throw upan::exception(XLOC, "out of memory!");
}

bool DMM::_free(uintptr_t address) {
  if(address == NULL)
    return true;

  if(address <= _heapStartAddress)
    return false;

  auto freeAUT = (AllocationUnitTracker*)(address - sizeof(AllocationUnitTracker));
  uintptr_t allocatedAddress = address - sizeof(AllocationUnitTracker) - freeAUT->byteStuffForAlign;
  uintptr_t size = freeAUT->size;
  const uint64_t calcCheckSum = freeAUT->calculateCheckSum();
  if (calcCheckSum != freeAUT->checkSum) {
    throw upan::exception(XLOC, "bad address dealloc %llx", address);
  }
  if (freeAUT->returnAddress == NULL) {
    throw upan::exception(XLOC, "double delete %llx", address);
  }
  freeAUT = (AllocationUnitTracker*)allocatedAddress;
  freeAUT->allocatedAddress = freeAUT;
  freeAUT->returnAddress = NULL;
  freeAUT->size = size;
  freeAUT->nextAUTAddress = nullptr;
// old algorithm:- sorted free list with merging
//  freeAUT->nextAUTAddress = _rootAut;
//  _rootAut = freeAUT;
//  freeAUT->updateCheckSum();
//  return true;

  if (freeAUT->allocatedAddress < _rootAut) {
    freeAUT->nextAUTAddress = _rootAut;
    _rootAut = freeAUT;
    freeAUT->updateCheckSum();
  } else {
    for (auto aut = _rootAut; aut != nullptr; aut = aut->nextAUTAddress) {
      AllocationUnitTracker* nextAut = aut->nextAUTAddress;
      if (nextAut == nullptr) {
        aut->nextAUTAddress = freeAUT;
        aut->updateCheckSum();
        freeAUT->updateCheckSum();
        break;
      } else if (aut->allocatedAddress < freeAUT->allocatedAddress &&
                 freeAUT->allocatedAddress < nextAut->allocatedAddress) {
        aut->nextAUTAddress = freeAUT;
        freeAUT->nextAUTAddress = nextAut;
        aut->updateCheckSum();
        freeAUT->updateCheckSum();
        break;
      }
    }
  }

  AllocationUnitTracker* prevAut = nullptr;
  for(auto aut = _rootAut; aut != nullptr;)	{
    if (aut->nextAUTAddress != nullptr) {
      if (((uint64_t) aut + aut->size) == (uint64_t) aut->nextAUTAddress) {
        aut->size += aut->nextAUTAddress->size;
        aut->nextAUTAddress = aut->nextAUTAddress->nextAUTAddress;
        aut->updateCheckSum();
      } else if (((uint64_t) aut->nextAUTAddress + aut->nextAUTAddress->size) == (uint64_t) aut) {
        aut->nextAUTAddress->size += aut->size;
        aut = aut->nextAUTAddress;
        if (prevAut) {
          prevAut->nextAUTAddress = aut;
        } else {
          _rootAut = aut;
        }
        aut->updateCheckSum();
      } else {
        prevAut = aut;
        aut = aut->nextAUTAddress;
      }
    } else {
      prevAut = aut;
      aut = aut->nextAUTAddress;
    }
  }

  return true;
}

bool DMM::_getAllocSize(uintptr_t address, size_t* size) {
  if (address == NULL || address < sizeof(AllocationUnitTracker) || address == _heapStartAddress) {
    *size = 0;
    return false;
  }
  auto aut = (AllocationUnitTracker *) (address - sizeof(AllocationUnitTracker));
  *size = aut->size;
  return true;
}

uint64_t DMM::_availableHeapSize() {
	uint64_t size = 0 ;
	uint64_t total_chunks = 0;
	char buf[512];
	for(auto aut = _rootAut; aut != nullptr; aut = aut->nextAUTAddress)	{
    size += aut->size;
    ++total_chunks;
    sprintf(buf, "\n Chunk: %u, %u, %u", aut->allocatedAddress, aut->size, (uint64_t)aut->allocatedAddress + aut->size);
    //COM1::Instance().Write(buf);
    printf("%s", buf);
	}
	printf("\n Total chunks: %d", total_chunks);
	return size ;
}

UserDMM::UserDMM() : DMM(PROCESS_HEAP_START_ADDRESS, PROCESS_HEAP_SIZE) {}

uintptr_t UserDMM::allocate(uint32_t sizeInBytes, uint32_t alignment) {
  upan::mutex_guard g(_mutex);
  return _allocate(sizeInBytes, alignment);
}

void UserDMM::accessMem(AllocationUnitTracker *aut) {
  //Make sure that all pages are allocated in the requested mem block
  auto addr = (uintptr_t)aut->allocatedAddress;
  const auto endAddr = addr + aut->size;
  UNUSED __volatile__ int x;
  while (addr < endAddr) {
    x = ((char *) addr)[0]; // A read would cause a page fault
    addr += PAGE_SIZE;
  }
  x = ((char *) (endAddr - 1))[0];
}

bool UserDMM::free(uintptr_t address) {
  upan::mutex_guard g(_mutex);
  return _free(address);
}
bool UserDMM::getAllocSize(uintptr_t address, size_t* size) {
  upan::mutex_guard g(_mutex);
  return _getAllocSize(address, size);
}

uint64_t UserDMM::availableHeapSize() {
  upan::mutex_guard g(_mutex);
  return _availableHeapSize();
};

KernelDMM::KernelDMM() : DMM(MEM_KERNEL_HEAP_START, MEM_KERNEL_HEAP_SIZE) {
}

uintptr_t KernelDMM::allocate(uint32_t sizeInBytes, uint32_t alignment) {
  IrqGuard g;
  return _allocate(sizeInBytes, alignment);
}

bool KernelDMM::free(uintptr_t address) {
  IrqGuard g;
  return _free(address);
}

bool KernelDMM::getAllocSize(uintptr_t address, size_t* size) {
  IrqGuard g;
  return _getAllocSize(address, size);
}

uint64_t KernelDMM::availableHeapSize() {
  IrqGuard g;
  return _availableHeapSize();
};
