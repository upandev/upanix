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

#include <ThreadLocalStorage.h>
#include <MemManager.h>

ThreadLocalStorage::ThreadLocalStorage(int pid, uint64_t* pml4Table,
                                       ThreadLocalSpace& tlsp, uint8_t pageFlag)
                                       : _pml4Table(pml4Table), _tlsp(tlsp), _pageFlag(pageFlag) {
  auto pml4Index = PML4_INDEX(THREAD_LOCAL_META_SPACE_ADDRESS);
  if (!PAGE_IS_PRESENT(pml4Table, pml4Index)) {
    auto pdpPage = MemManager::Instance().AllocatePhysicalPage();
    pml4Table[pml4Index] = (pdpPage * PAGE_SIZE) | _pageFlag;
  }

  //Do not update the PDP table entry to point to this thread's PD table here
  //Because this code is executed while creating this thread in the process space of kernel
  //So, we will end-up modifying the PDP entry of the running process
  //Updating PDP entry to point to this thread's PD table will happen in switchSpace()
  //at the time when this thread/process is loaded for execution during context switch
  _tlPDTable = (uint64_t*)(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  _tcb = (ThreadControlBlock*)getPageAddress(THREAD_LOCAL_META_SPACE_ADDRESS);

  _tcb->_self = THREAD_LOCAL_META_SPACE_ADDRESS;
  _tcb->_tlms._pid = pid;
  //the size of dtv itself is used as the generation-id
  _tcb->_dtv[0] = 0;

  update();
}

ThreadLocalStorage::~ThreadLocalStorage() {
  MemManager::Instance().DeallocatePDAddressSpace(_tlPDTable);
}

uintptr_t ThreadLocalStorage::getPageAddress(uint64_t address) {
  auto pdIndex = PD_INDEX(address);
  if (!PAGE_IS_PRESENT(_tlPDTable, pdIndex)) {
    _tlPDTable[pdIndex] = (uintptr_t) (MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE) | _pageFlag;
    printf("\n Allocating DTV PT");
  }

  auto ptTable = PAGE_TABLE(_tlPDTable, pdIndex);
  auto ptIndex = PT_INDEX(address);
  if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
    ptTable[ptIndex] = (uintptr_t) (MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE) | _pageFlag;
    printf("\n Allocating DTV PAGE");
  }

  return PAGE_ADDRESS(ptTable, ptIndex);
}

void ThreadLocalStorage::update() {
  auto& dtv = _tlsp.getDTV();

  if ((int)_tcb->_dtv[0] == dtv.size()) {
    return;
  }

  uint64_t offset = 0;
  for (int i = 0; i < (int)_tcb->_dtv[0]; ++i) {
    offset += dtv[i].first;
  }

  printf("\n DTV offset = %x", offset);

  for (int i = (int)_tcb->_dtv[0]; i < dtv.size(); ++i) {
    offset += dtv[i].first;
    allocate(i, offset, dtv[i]);
  }
  _tcb->_dtv[0] = dtv.size();
}

void ThreadLocalStorage::allocate(int index, uint64_t offset, ThreadLocalSpace::DTV_LIST::element_type& dtv) {
  if (index + 1 == MAX_DTV_SIZE) {
    throw upan::exception(XLOC, "TLS is out-of-memory - count cap reached");
  }

  if (offset >= ((1 GB) - (4 KB))) {
    throw upan::exception(XLOC, "TLS is out-of-memory - alloc cap reached");
  }

  uint64_t address = THREAD_LOCAL_META_SPACE_ADDRESS - offset;
  _tcb->_dtv[index + 1] = address;

  printf("\n Allocating DTV for %d @ %d", index, offset);
  auto remainingLen = dtv.first;
  while (remainingLen > 0) {
    const auto copy_offset = PAGE_OFFSET(address);

    auto copy_len = PAGE_SIZE - copy_offset;
    if (copy_len > remainingLen) copy_len = remainingLen;

    const auto copy_address = getPageAddress(address) + copy_offset;

    printf("\n Copying DTV module @ %lx with offset:%d for len: %d (remaining: %d)", copy_address, copy_offset, copy_len, remainingLen);

    if (dtv.second) {
      memcpy((uint8_t*)copy_address, dtv.second + dtv.first - remainingLen, copy_len);
    } else {
      memset((uint8_t*)copy_address, 0, copy_len);
    }
    remainingLen -= copy_len;
  }
}

void ThreadLocalStorage::setPDAddress(uintptr_t value) {
  const auto pml4Index = PML4_INDEX(THREAD_LOCAL_META_SPACE_ADDRESS);
  auto pdpTable = PAGE_TABLE(_pml4Table, pml4Index);
  const auto pdpIndex = PDP_INDEX(THREAD_LOCAL_META_SPACE_ADDRESS);
  pdpTable[pdpIndex] = value;
}

void ThreadLocalStorage::switchSpace() {
  update();
  setPDAddress(((uintptr_t)_tlPDTable & PAGE_MASK) | _pageFlag);
}