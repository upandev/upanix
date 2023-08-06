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

#include <KernelThread.h>
#include <KernelProcess.h>
#include <MemManager.h>

KernelThread::KernelThread(KernelProcess& parent, uint32_t threadCaller, uint32_t entryAddress, void* arg)
  : Thread(parent) {
  const uint32_t uiStackAddress = AllocateAddressSpace();
  const uint32_t uiStackTop = uiStackAddress - GLOBAL_DATA_SEGMENT_BASE + (PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE) - 1;
  upan::vector<uintptr_t> params;
  params.push_back(entryAddress);
  params.push_back((uintptr_t)arg);
  _taskState.BuildForKernel(threadCaller, uiStackTop, params);
  _processLDT.BuildForKernel();

  _parent.addToThreadScheduler(*this);
}

uint64_t* KernelThread::AllocateAddressSpace() {
  kernelStackBlockId = MemManager::Instance().AllocateKernelStack();
  return MemManager::Instance().GetKernelStackAddress(kernelStackBlockId);
}

void KernelThread::DeallocateResources() {
  MemManager::Instance().DeAllocateKernelStack(kernelStackBlockId);
}