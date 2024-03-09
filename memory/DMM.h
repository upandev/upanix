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
#pragma once

#include <Global.h>
#include <MemConstants.h>
#include <ProcessConstants.h>
#include <mutex.h>

#define NULL 0x0

typedef struct AllocationUnitTracker {
  AllocationUnitTracker* allocatedAddress;
  uintptr_t returnAddress;
  uint64_t size;
  uint64_t checkSum;
  union {
    AllocationUnitTracker* nextAUTAddress;
    uint32_t byteStuffForAlign;
  };

  uint64_t calculateCheckSum() const;
  void updateCheckSum();
} PACKED AllocationUnitTracker ; // AUT

class DMM {
protected:
  DMM(uint64_t heapStartAddress, uint64_t heapMaxSize);

public:
  bool isDmmFlag() const { return _dmmFlag; }
  virtual uintptr_t allocate(uint32_t sizeInBytes, uint32_t alignment = 0) = 0;
  virtual bool free(uintptr_t address) = 0;
  virtual bool getAllocSize(uintptr_t address, size_t* size) = 0;
  virtual uint64_t availableHeapSize() = 0;
  virtual void releaseLocks(int pid) {}

protected:
  uintptr_t _allocate(uint32_t sizeInBytes, uint32_t alignment);
  bool _free(uintptr_t address);
  bool _getAllocSize(uintptr_t address, size_t* size);
  uint64_t _availableHeapSize();
  virtual void accessMem(AllocationUnitTracker*) {}

private:
  uint32_t getByteStuffForAlign(uint64_t uiAddress, uint32_t uiAlignNumber) const;
  void validateAlignParam(uint32_t alignment) const;

private:
  const uint64_t _heapStartAddress;
  const uint64_t _heapMaxSize;
  bool _dmmFlag;
  AllocationUnitTracker* _rootAut;
};

class UserDMM : public DMM {
public:
  UserDMM();
  uintptr_t allocate(uint32_t sizeInBytes, uint32_t alignment) override;
  bool free(uintptr_t address) override;
  bool getAllocSize(uintptr_t address, size_t* size) override;
  uint64_t availableHeapSize() override;
  void releaseLocks(int pid) override { _mutex.unlock(pid); }

private:
  void accessMem(AllocationUnitTracker* aut) override;

  upan::mutex _mutex;
};

class KernelDMM : public DMM {
private:
  KernelDMM();
public:
  static DMM& Instance() {
    static KernelDMM dmm;
    return dmm;
  }
  uintptr_t allocate(uint32_t sizeInBytes, uint32_t alignment) override;
  bool free(uintptr_t address) override;
  bool getAllocSize(uintptr_t address, size_t* size) override;
  uint64_t availableHeapSize() override;
};