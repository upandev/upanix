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

#include <UserThread.h>
#include <ProcessManager.h>

//thread must have a parent
UserThread::UserThread(AutonomousProcess& parent, uintptr_t threadCaller, uintptr_t entryAddress, void* arg)
  : Thread(parent) {
  _stackPDAddress = SchedulableProcess::Common::AllocateStackSpace();
  //call return address, unused - the thread function is a typical c function and expects the return address to be the first entry on top of call stack
  //but a thread function - unlike a typical c function, should exit() instead of return
  const auto stackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_SYSCALL_STACK_SIZE - sizeof(uint64_t);

  _taskContext.rdi = entryAddress;
  _taskContext.rsi = (uintptr_t)arg;

  _taskContext.interruptState.cs = USER_CODE_SELECTOR | 0x3;
  _taskContext.interruptState.rip = threadCaller;
  _taskContext.interruptState.ss = USER_DATA_SELECTOR | 0x3;
  _taskContext.interruptState.rsp = stackTopAddress;
  _taskContext.interruptState.rflags = 0x202;

  _parent.addToThreadScheduler(*this);
}

void UserThread::DeallocateResources() {
  SchedulableProcess::Common::DeAllocateStackSpace(_stackPDAddress);
  MemManager::Instance().DeAllocatePhysicalPage(_stackPDAddress / PAGE_SIZE);
}

void UserThread::onLoad() {
  SchedulableProcess::Common::SwitchStack(pml4Table(), _stackPDAddress);
}