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

#include <SchedulableProcess.h>
#include <MountManager.h>
#include <UserManager.h>
#include <ProcessGroup.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <Cpu.h>
#include <thread_context.h>
#include <StorageDriveManager.h>

int SchedulableProcess::_nextPid = 0;

SchedulableProcess::SchedulableProcess(const upan::string& name, int parentID, bool isFGProcess)
  : _name(name), _stateInfo(*new ProcessStateInfo()), _processGroup(nullptr) {
  _processID = _nextPid++;
  _parentProcessID = parentID;
  _status = NEW;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);

  if(parentProcess.isEmpty()) {
    _driveID = ROOT_DRIVE_ID ;
    if(_driveID != CURRENT_DRIVE)
    {
      StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(_driveID, false).goodValueOrThrow(XLOC);
      if(diskDrive.Mounted()) {
        _pwd = diskDrive.fileSystem().root();
      }
    }
    _processGroup = new ProcessGroup(isFGProcess);
  } else {
    _driveID = parentProcess.value()._driveID ;
    _pwd = parentProcess.value().pwd();
    _processGroup = parentProcess.value()._processGroup;
  }

  _processGroup->AddProcess();
  if(isFGProcess)
    _processGroup->PutOnFGProcessList(_processID);

  AllocateInterruptStackSpace();
}

SchedulableProcess::~SchedulableProcess() {
  delete &_stateInfo;
}

// 1. KernelProcess can have child processes of type either KernelProcess or KernelThread or UserProcess
// 2. KernelThread can have a child KernelProcess or KernelThread or UserProcess
// 3. UserProcess can have a child UserProcess or UserThread
// 4. UserThread can have a child UserProcess or UserThread
// 5. Thread created by another Thread will have its parent set to the parent of the creator Thread (which will be an AutonomousProcess - main thread)
// 6. If a parent process (Kernel or User) is terminated then
//   a. all terminated child processes are released and all non-terminated child processes are redirected to the parent of the current process
//   b. all child threads are destroyed and released
// 7. If a child thread terminates then
//   a. all terminated child processes are released and all non-terminated child processes are redirected to main thread process
//   b. as per (5), there can't be any child threads under another child thread
void SchedulableProcess::Destroy() {
  setStatus(TERMINATED);

  DestroyThreads();

  // child processes of this process (if any) will be redirected to the parent of the current process
  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(_parentProcessID);
  for(auto pid : _childProcessIDs) {
    ProcessManager::Instance().GetSchedulableProcess(pid).ifPresent([&parentProcess](SchedulableProcess &p) {
      if (p.status() == TERMINATED) {
        p.Release();
      } else {
        parentProcess.ifPresent([&p](SchedulableProcess& pp) {
          p.setParentProcessID(pp.processID());
          pp.addChildProcessID(p.processID());
        });
      }
    });
  }

  // Deallocate Resources
  Deallocate();

  // Release From Process Group
  _processGroup->RemoveFromFGProcessList(_processID);
  _processGroup->RemoveProcess();

  if(_processGroup->Size() == 0)
    delete _processGroup;

  dmm().releaseLocks(_processID);
  pageAllocMutex().ifPresent([this](upan::mutex& m) { m.unlock(_processID); });

  //TODO: release all the mutex held by the process or an individual thread

  if(_parentProcessID == NO_PROCESS_ID) {
    Release();
  }
}

void SchedulableProcess::Release() {
  setStatus(RELEASED);
}

void SchedulableProcess::yield() {
  do {
    __asm__ __volatile__ ("int $0x20");
  } while (status() != RUN);
}

bool SchedulableProcess::CanPreempt() {
  return (PIT::Instance().GetClockCount() - _runTick) > 10;
}

void SchedulableProcess::Load(TaskContext& taskContext) {
  _runTick = PIT::Instance().GetClockCount();
  _tls->switchSpace();
  SwitchInterruptStack();
  onLoad();

  //switch thread-local space
  //don't deallocate page-table entries for thread-local space for kernel processes as it is shared across all kernel processes
//  auto ptTable = MemManager::Instance().GetPTTable(pml4Table(), upan::thread_context::SHARED_ADDRESS);
//  auto ptIndex = PT_INDEX(upan::thread_context::SHARED_ADDRESS);
//  ptTable[ptIndex] = (_threadContextPageNumber * PAGE_SIZE) | (isKernelProcess() ? 0x3 : 0x7);

  taskContext = _taskContext;
}

void SchedulableProcess::Deallocate() {
  DeAllocateInterruptStackSpace();
  DeallocateResources();
}

void SchedulableProcess::SwitchInterruptStack() {
  uintptr_t istVirtualAddress = MEM_KERNEL_IST3_COMMON_STACK_TOP - _istStackPages.size() * PAGE_SIZE;
  for(auto istRealAddress : _istStackPages) {
    MemManager::Instance().MapAddressSpace(pml4Table(), 0x7, istVirtualAddress, istRealAddress, PAGE_SIZE);
    istVirtualAddress += PAGE_SIZE;
  }
}

void SchedulableProcess::AllocateInterruptStackSpace() {
  //Allocate stack space for Interrupts, which is used by all ISTs which don't have their dedicated stack space like timer and page-fault
  //Not used by syscall because syscall has its own stack management
  const int MEM_INTERRUPT_STACK_PAGE_COUNT = 8; // 32 KB
  for (int i = 0; i < MEM_INTERRUPT_STACK_PAGE_COUNT; ++i) {
    _istStackPages.push_back(MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE);
  }
}

void SchedulableProcess::DeAllocateInterruptStackSpace() {
  for(auto pageAddress : _istStackPages) {
    MemManager::Instance().DeAllocatePhysicalPage(pageAddress / PAGE_SIZE);
  }
  _istStackPages.clear();
}

void SchedulableProcess::switchPageTable() const {
  Cpu::SetRegValue(Cpu::CR3, (uint64_t)pml4Table());
}

void SchedulableProcess::Store(const TaskContext& taskContext) {
  _taskContext = taskContext;
}

FILE_USER_TYPE SchedulableProcess::fileUserType(const FileNode &node) const
{
  if(isKernelProcess() || _userID == ROOT_USER_ID || node.UserID() == _userID)
    return USER_OWNER ;

  return USER_OTHERS ;
}

bool SchedulableProcess::hasFilePermission(const FileNode& node, byte mode) const
{
  unsigned short usMode = FILE_PERM(node.Attribute());

  bool bHasRead, bHasWrite;

  switch(fileUserType(node))
  {
    case FILE_USER_TYPE::USER_OWNER:
      bHasRead = HAS_READ_PERM(G_OWNER(usMode));
      bHasWrite = HAS_WRITE_PERM(G_OWNER(usMode));
      break;

    case FILE_USER_TYPE::USER_OTHERS:
      bHasRead = HAS_READ_PERM(G_OTHERS(usMode));
      bHasWrite = HAS_WRITE_PERM(G_OTHERS(usMode));
      break;

    default:
      return false;
  }

  if(mode & O_RDONLY)
  {
    return bHasRead || bHasWrite;
  }
  else if((mode & O_WRONLY) || (mode & O_RDWR) || (mode & O_APPEND))
  {
    return bHasWrite;
  }
  return false;
}

void SchedulableProcess::Common::SetStackPDTable(uint64_t *pml4Table, uint64_t value) {
  auto pml4Index = PML4_INDEX(PROCESS_STACK_TOP_ADDRESS - 1);
  auto pdpTable = PAGE_TABLE(pml4Table, pml4Index);
  auto pdpIndex = PDP_INDEX(PROCESS_STACK_TOP_ADDRESS - 1);
  pdpTable[pdpIndex] = value;
}

void SchedulableProcess::Common::SwitchStack(uint64_t* pml4Table, uint64_t stackPDAddress) {
  SetStackPDTable(pml4Table, (stackPDAddress & PAGE_MASK) | 0x7);
}

uint64_t SchedulableProcess::Common::AllocateStackSpace() {
  //pre-allocate process stack - user (the initial space for start-args) + call-gate
  //further expansion of user stack beyond initial space for start-args will happen as part of regular page fault handling flow
  const uint64_t stackPDAddress = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  const uint64_t processSysCallStackBase = PROCESS_STACK_TOP_ADDRESS - PROCESS_SYSCALL_STACK_SIZE;
  const uint64_t processStackBase = processSysCallStackBase - PROCESS_INIT_STACK_SIZE;
  MemManager::Instance().AllocatePDAddressSpace((uint64_t*)stackPDAddress, 0x7, processStackBase, PROCESS_INIT_STACK_SIZE);
  MemManager::Instance().AllocatePDAddressSpace((uint64_t*)stackPDAddress, 0x7, processSysCallStackBase, PROCESS_SYSCALL_STACK_SIZE);

  return stackPDAddress;
}

void SchedulableProcess::Common::DeAllocateStackSpace(uint64_t stackPDAddress) {
  MemManager::Instance().DeallocatePDAddressSpace((uint64_t*)stackPDAddress);
}

uint64_t SchedulableProcess::Common::KernelVirtualStackBase(int stackBlockId) {
  return PROCESS_KERNEL_STACK_BASE + stackBlockId * PROCESS_KERNEL_STACK_SIZE;
}

int SchedulableProcess::Common::AllocateKernelStackSpace() {
  int stackBlockId = MemManager::Instance().AllocateKernelStack();
  MemManager::Instance().AllocateAddressSpace(MEM_PML4_TABLE, 0x3, KernelVirtualStackBase(stackBlockId), PROCESS_KERNEL_STACK_SIZE);
  return stackBlockId;
}

void SchedulableProcess::Common::DeallocateKernelStackSpace(int stackBlockId) {
  MemManager::Instance().DeallocateAddressSpace(MEM_PML4_TABLE, KernelVirtualStackBase(stackBlockId), PROCESS_KERNEL_STACK_SIZE);
  MemManager::Instance().DeAllocateKernelStack(stackBlockId);
}

extern __volatile__ uint64_t SYS_CALL_ID;

bool SchedulableProcess::handlePageFault(uint64_t faultyAddress) {
  Cpu::SetRegValue(Cpu::CR3, (uint64_t)MEM_PML4_TABLE);

  const auto virtualPageNo = faultyAddress / PAGE_SIZE;
  if (isKernelProcess()) {
    printf("\n Page Fault in Kernel! FIX THIS !!!");
    printf("\n Page Fault Address/Page: %llx / %u", faultyAddress, virtualPageNo);
    __asm__ __volatile__ ("HLT");
    while (true);
  }

  bool permittedAddressAccess = false;
  //This space is for process Stack - page fault here should be only while expanding stack and not for Heap (DMM is OFF)
  if (faultyAddress >= (PROCESS_STACK_TOP_ADDRESS - PROCESS_STACK_SIZE)
      && faultyAddress < PROCESS_STACK_TOP_ADDRESS
      && !dmm().isDmmFlag()) {
    permittedAddressAccess = true;
  } //page fault in heap while allocating memory (DMM is ON)
  else if (faultyAddress >= PROCESS_HEAP_START_ADDRESS
           && faultyAddress < (PROCESS_HEAP_START_ADDRESS + PROCESS_HEAP_SIZE)
           && dmm().isDmmFlag()) {
    permittedAddressAccess = true;
  }

  if (!permittedAddressAccess) {
    printf("\n Segmentation Fault @ Address: 0x%llx", faultyAddress);
    printf("\n Sys Call Id: %lu", SYS_CALL_ID);
    printf("\n PID: %d, DMM Flag: %d", _processID, dmm().isDmmFlag());
    switchPageTable();
    return false;
  }

  auto ptTable = MemManager::Instance().GetPTTable(pml4Table(), faultyAddress);
  const auto ptIndex = PT_INDEX(faultyAddress);
  auto address = ptTable[ptIndex];

  if ((address & 0x1) == 0) {
    auto page = MemManager::Instance().AllocatePhysicalPage();
    ptTable[ptIndex] = (page * PAGE_SIZE) | 0x7;
  } else if ((address & 0x7) == 0x7) {
    // we are good - page is already allocated - possibly because of a page fault on same address/page area from another thread.
  } else {
    /* Crash the Process..... With SegFault Or OutOfMemeory Error*/
    printf("\n Segmentation/Permission Fault @ Address: 0x%lx", faultyAddress);
    switchPageTable();
    return false;
  }

  switchPageTable();
  return true;
}