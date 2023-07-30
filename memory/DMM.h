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
#ifndef _DMM_H_
#define _DMM_H_

#include <Global.h>
#include <MemConstants.h>
#include <mutex.h>

#define NULL 0x0

#define DMM_SUCCESS				0
#define DMM_BAD_DEALLOC			1
#define DMM_BAD_ALIGN			2
#define DMM_FAILURE				3

#define PROCESS_VIRTUAL_ALLOCATED_ADDRESS(RealAddress) ((uintptr_t)(RealAddress) - PROCESS_BASE)
#define PROCESS_REAL_ALLOCATED_ADDRESS(VirtualAddress) ((uintptr_t)(VirtualAddress) + PROCESS_BASE)

typedef struct AllocationUnitTracker {
  AllocationUnitTracker* allocatedAddress;
  uintptr_t returnAddress;
  uint64_t size;
  uint64_t checkSum;
  union {
    AllocationUnitTracker* nextAUTAddress;
    uint32_t byteStuffForAlign;
  };
} PACKED AllocationUnitTracker ; // AUT

class Process;
class SchedulableProcess;

uintptr_t DMM_Allocate(Process* processAddressSpace, unsigned sizeInBytes, unsigned alignNumber = 0);

void DMM_InitAUTForKernel();
uintptr_t DMM_AllocateForKernel(unsigned sizeInBytes, unsigned alignNumber = 0);

byte DMM_DeAllocate(Process* processAddressSpace, uintptr_t address);
bool DMM_DeAllocateForKernel(uintptr_t address);

bool DMM_GetAllocSize(uintptr_t address, size_t* size);
bool DMM_GetAllocSizeForKernel(uintptr_t address, size_t* size);
void DMM_DeAllocatePhysicalPages(Process* processAddressSpace);
unsigned DMM_KernelHeapAllocSize() ;

#endif
