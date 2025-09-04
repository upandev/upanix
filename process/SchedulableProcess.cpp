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
#include <UserManager.h>
#include <ProcessGroup.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <Cpu.h>
#include <StorageDriveManager.h>
#include <KernelRootProcess.h>
#include <StorageDrive.h>

int SchedulableProcess::_nextPid = 0;

SchedulableProcess::SchedulableProcess(const upan::string& name, int parentID, bool isFGProcess)
  : _name(name), _stateInfo(*new ProcessStateInfo()), _processGroup(nullptr), _signalQueue(MAX_ACTIVE_SIGNALS) {
  _processID = _nextPid++;
  _parentProcessID = parentID;
  _status = NEW;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);

  if(parentProcess.isEmpty()) {
    _driveID = CURRENT_DRIVE;
    StorageDriveManager::Instance().GetRootDrive().ifPresent([&](StorageDrive& drive) {
      _driveID = drive.Id();
      if (drive.Mounted()) {
        _pwd = drive.fileSystem().root();
      }
    });
    _processGroup = new ProcessGroup(isFGProcess);
    sigemptyset(&_sigMask);
  } else {
    _driveID = parentProcess.value()._driveID;
    _pwd = parentProcess.value().pwd();
    _processGroup = parentProcess.value()._processGroup;
    _sigMask = parentProcess.value()._sigMask;
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
  } else {
    if (!isChildThread() && !parentProcess.isEmpty()) {
      auto signalHandler = parentProcess.value().getSignalAction(SIGCHLD);
      if (signalHandler.isEmpty() || isignoreaction(&signalHandler.value()) || isdefaultaction(&signalHandler.value())) {
        Release();
      } else {
        union sigval sigval {_processID };
        ProcessManager::Instance().SendSignal(_parentProcessID, SIGCHLD, &sigval);
        if (signalHandler.value().sa_flags & SA_NOCLDWAIT) {
          Release();
        }
      }
    }
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
  auto pml4Index = PML4_INDEX(PROCESS_STACK_BASE - 1);
  auto pdpTable = PAGE_TABLE(pml4Table, pml4Index);
  auto pdpIndex = PDP_INDEX(PROCESS_STACK_BASE - 1);
  pdpTable[pdpIndex] = value;
}

void SchedulableProcess::Common::SwitchStack(uint64_t* pml4Table, uint64_t stackPDAddress) {
  SetStackPDTable(pml4Table, (stackPDAddress & PAGE_MASK) | 0x7);
}

uint64_t SchedulableProcess::Common::AllocateStackSpace() {
  //pre-allocate process stack - user (the initial space for start-args) + call-gate
  //further expansion of user stack beyond initial space for start-args will happen as part of regular page fault handling flow
  const uint64_t stackPDAddress = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  const uint64_t processSyscallReserveSpaceBase = PROCESS_STACK_BASE - PROCESS_SYSCALL_RESERVE_SPACE;
  MemManager::Instance().AllocatePDAddressSpace((uint64_t*)stackPDAddress, 0x7, processSyscallReserveSpaceBase, PROCESS_SYSCALL_RESERVE_SPACE);
  const uint64_t processStackBase = PROCESS_STACK_TOP_ADDRESS - PROCESS_INIT_STACK_SIZE;
  MemManager::Instance().AllocatePDAddressSpace((uint64_t*)stackPDAddress, 0x7, processStackBase, PROCESS_INIT_STACK_SIZE);

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
  MemManager::Instance().AllocateAddressSpace(MEM_PML4_TABLE, 0x3, KernelVirtualStackBase(stackBlockId) - PROCESS_KERNEL_STACK_SIZE, PROCESS_KERNEL_STACK_SIZE);
  return stackBlockId;
}

void SchedulableProcess::Common::DeallocateKernelStackSpace(int stackBlockId) {
  MemManager::Instance().DeallocateAddressSpace(MEM_PML4_TABLE, KernelVirtualStackBase(stackBlockId) - PROCESS_KERNEL_STACK_SIZE, PROCESS_KERNEL_STACK_SIZE);
  MemManager::Instance().DeAllocateKernelStack(stackBlockId);
}

uint64_t SchedulableProcess::Common::CalculateSignalFrameRSP(uint64_t rsp, bool hasSiginfo) {
  rsp = upan::align_down(rsp, 16);
  rsp -= 128; //skip red-zone
  if (hasSiginfo) {
    rsp -= sizeof(siginfo_t);
  }
  rsp -= sizeof(SignalTaskContext);
  rsp -= sizeof(uint64_t); //the signal restorer address
  return rsp;
}

void SchedulableProcess::Common::initSignalFrame(SchedulableProcess& process, uint64_t rsp, const struct sigaction& action, const Signal& signal) {
  TaskContext& taskContext = process._taskContext;

  auto signalFrame = (uint8_t*)rsp;
  int framePos = 0;

  memcpy(signalFrame, (void*)&action.sa_restorer, sizeof(uint64_t));
  framePos += sizeof(uint64_t);

  SignalTaskContext signalTaskContext { taskContext, signal.signo(), process.status(), process.signalMask() };
  memcpy(signalFrame + framePos, (void*)&signalTaskContext, sizeof(SignalTaskContext));
  framePos += sizeof(SignalTaskContext);

  if (action.sa_flags & SA_SIGINFO) {
    siginfo_t siginfo;
    siginfo.si_signo = signal.signo();
    siginfo.si_value = signal.value();
    switch(signal.signo()) {
      //TODO: set other info fields based on the signal
    }

    memcpy(signalFrame + framePos, (void*)&siginfo, sizeof(siginfo_t));
    taskContext.rsi = rsp + framePos;

    taskContext.interruptState.rip = (uint64_t)action.sa_sigaction;
  } else {
    taskContext.interruptState.rip = (uint64_t)action.sa_handler;
  }

  taskContext.rdi = signal.signo();
  taskContext.rdx = 0;

  taskContext.interruptState.rsp = rsp;
  taskContext.interruptState.rflags = 0x202;
}

void SchedulableProcess::Common::SetupSignalStackFrame(SchedulableProcess& process, uint64_t stackPDAddress,
                                                       const struct sigaction& action, const Signal& signal) {
  TaskContext& taskContext = process._taskContext;
  const uint64_t rsp = CalculateSignalFrameRSP(taskContext.interruptState.rsp, action.sa_flags & SA_SIGINFO);

  if ((PROCESS_STACK_TOP_ADDRESS - rsp) >= PROCESS_STACK_SIZE) {
    throw upan::exception(XLOC, "out of stack space - can't allocate signal frame");
  }

  const uint64_t signalFrameSize = taskContext.interruptState.rsp - rsp;
  if (signalFrameSize > PAGE_SIZE) {
    throw upan::exception(XLOC, "signal frame size (%d) > PAGE_SIZE - can't allocate signal frame", signalFrameSize);
  }

  auto ptTable = MemManager::Instance().GetPTTableFromPD((uint64_t*)stackPDAddress, rsp);
  const auto ptIndex = PT_INDEX(rsp);

  if (!PAGE_IS_PRESENT(ptTable, ptIndex)) {
    auto page = MemManager::Instance().AllocatePhysicalPage();
    ptTable[ptIndex] = (page * PAGE_SIZE) | 0x7;
  }

  process.switchPageTable();
  SwitchStack(process.pml4Table(), stackPDAddress);
  //auto signalFrame = (uint8_t*)MemManager::Instance().GetFlatAddressFromPD((uint64_t*)stackPDAddress, rsp);
  initSignalFrame(process, rsp, action, signal);
  KernelRootProcess::Instance().switchPageTable();

  taskContext.interruptState.cs = USER_CODE_SELECTOR | 0x3;
  taskContext.interruptState.ss = USER_DATA_SELECTOR | 0x3;
}

void SchedulableProcess::Common::SetupKernelSignalStackFrame(SchedulableProcess& process, int stackBlockId,
                                                             const struct sigaction& action, const Signal& signal) {
  TaskContext& taskContext = process._taskContext;
  const uint64_t rsp = CalculateSignalFrameRSP(taskContext.interruptState.rsp, action.sa_flags & SA_SIGINFO);

  if ((KernelVirtualStackBase(stackBlockId) - rsp) >= PROCESS_KERNEL_STACK_SIZE) {
    throw upan::exception(XLOC, "out of stack space - can't allocate signal frame");
  }

  initSignalFrame(process, rsp, action, signal);

  taskContext.interruptState.cs = SYS_CODE_SELECTOR;
  taskContext.interruptState.ss = SYS_DATA_SELECTOR;
}

extern __volatile__ uint64_t SYS_CALL_ID;

bool SchedulableProcess::handlePageFault(TaskContext& taskContext, uint64_t faultyAddress) {
  KernelRootProcess::Instance().switchPageTable();

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
    union sigval value;
    value.sival_ptr = (void*)faultyAddress;
    queueSignal(SIGSEGV, &value);

    printf("\n Segmentation Fault @ Address: 0x%llx", faultyAddress);
    printf("\n Sys Call Id: %lu", SYS_CALL_ID);
    printf("\n PID: %d, DMM Flag: %d", _processID, dmm().isDmmFlag());
    switchPageTable();

    //move the process to Preempted state, that will make sure SIGSEGV is delivered before executing the user space code again
    setStatus(PREEMPTED);
    //switch stack to user process stack - as the signal frame has to be constructed on the user-stack only
    __asm__ __volatile__("mov %%rsp, %0;"
                         "mov %1, %%rsp;"
                         : "=m"(_pageFaultRSPBackup)
                         : "r"(taskContext.interruptState.rsp)
                         : "rsp");
    __asm__ __volatile__("int $0x20");
    __asm__ __volatile__("mov %0, %%rsp;" : : "m"(_pageFaultRSPBackup) : "rsp");
    return true;
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
    /* Crash the Process... With SegFault Or OutOfMemory Error*/
    printf("\n Segmentation/Permission Fault @ Address: 0x%llx", faultyAddress);
    switchPageTable();
    return false;
  }

  switchPageTable();
  return true;
}

void SchedulableProcess::maskSignal(SIG_MASKING_TYPE how, const sigset_t* set, sigset_t* oldset) {
  if (oldset) {
    *oldset = _sigMask;
  }

  switch(how) {
    case SIG_BLOCK: {
      if (set) {
        for(int i = 0; i < _NSIG_WORDS; ++i) {
          _sigMask.__val[i] |= set->__val[i];
        }
      }
    }
    break;

    case SIG_UNBLOCK: {
      if (set) {
        for(int i = 0; i < _NSIG_WORDS; ++i) {
          _sigMask.__val[i] &= ~(set->__val[i]);
        }
      }
    }
    break;

    case SIG_SETMASK: {
      _sigMask = *set;
    }
    break;

    default:
      throw upan::exception(XLOC, "invalid signal masking type: %d", how);
  }
}

//this is always called via ProcessManager::SendSignal - which locks context switch to protect the queue access across context switches
void SchedulableProcess::queueSignal(SIGNAL signo, const union sigval* value) {
  if (_signalQueue.full()) {
    throw upan::exception(XLOC, "signal queue is full - can't deliver signal %d", signo);
  }
  _signalQueue.push_back({signo, value});
}

upan::option<Signal> SchedulableProcess::getSignal() {
  if (_signalQueue.empty()) {
    return upan::option<Signal>::empty();
  }
  auto signal = _signalQueue.front();
  _signalQueue.pop_front();
  return upan::option<Signal>(signal);
}

bool SchedulableProcess::WakeupProcessOnInterrupt() {
  const IRQ& irq = *_stateInfo.Irq();

  if(irq == StdIRQ::Instance().NO_IRQ)
    return true;

  return irq.Consume();
}

void SchedulableProcess::prepareToRun() {
  bool interruptedBySignal = false;
  if (_status == SIGNAL_RETURN) {
    interruptedBySignal = true;
    const auto signo = _signalRestoreContext._signal;

    _taskContext = _signalRestoreContext._context;
    _sigMask = _signalRestoreContext._sigMask;
    sigdelset(&_sigMask, signo);
    setStatus(_signalRestoreContext._processStatus);

    if (_status == STOPPED && signo == SIGCONT) {
      setStatus(RUN);
    }
  }

  switch(_status) {
    case PREEMPTED:
      setStatus(RUN);
      break;

    case RELEASED:
      break;

    case TERMINATED: {
      if (_parentProcessID == ProcessManager::UpanixKernelProcessID()) {
        Release();
      }
    }
    break;

    case WAIT_SLEEP: {
      if(PIT::Instance().GetClockCount() >= _stateInfo.SleepTime()
         || _processID == KernelRootProcess::Instance().scheduleRunnerPid())
      {
        _stateInfo.SleepTime(0);
        _stateInfo.setError(ProcessStateInfo::NO_ERROR);
        setStatus(RUN);
      } else if (interruptedBySignal) {
        _stateInfo.SleepTime(_stateInfo.SleepTime() - PIT::Instance().GetClockCount());
        _stateInfo.setError(ProcessStateInfo::INTERRUPTED);
        setStatus(RUN);
      }
    }
    break ;

    case WAIT_INT: {
      if (WakeupProcessOnInterrupt()) {
        setStatus(RUN);
      }
    }
    break;

    case WAIT_INT_WITH_TIMEOUT: {
      if (WakeupProcessOnInterrupt()) {
        setStatus(RUN);
      } else {
        if (PIT::Instance().GetClockCount() >= _stateInfo.SleepTime()) {
          _stateInfo.SleepTime(0);
          setStatus(RUN);
        }
      }
    }
    break;

    case WAIT_EVENT: {
      if(_stateInfo.IsEventCompleted()) {
        setStatus(RUN);
      }
    }
    break;

    case WAIT_IO_DESCRIPTORS: {
      const auto& result = iodTable().selectCheck(_stateInfo.GetIODescriptors());
      if (!result.empty()) {
        _stateInfo.SetIODescriptors(result);
        _stateInfo.setError(ProcessStateInfo::NO_ERROR);
        setStatus(RUN);
      } else {
        if (_stateInfo.SleepTime() && PIT::Instance().GetClockCount() >= _stateInfo.SleepTime()) {
          _stateInfo.SleepTime(0);
          _stateInfo.setError(ProcessStateInfo::TIMEOUT);
          setStatus(RUN);
        } else if (interruptedBySignal) {
          _stateInfo.SleepTime(0);
          _stateInfo.setError(ProcessStateInfo::INTERRUPTED);
          setStatus(RUN);
        }
      }
    }
    break;

    case WAIT_CHILD: {
      if(_stateInfo.WaitChildProcId() < 0) {
        _stateInfo.WaitChildProcId(NO_PROCESS_ID);
        setStatus(RUN);
      } else {
        auto childProcess = ProcessManager::Instance().GetSchedulableProcess(_stateInfo.WaitChildProcId());
        if(childProcess.isEmpty() || childProcess.value().parentProcessID() != _processID) {
          removeChildProcessID(_stateInfo.WaitChildProcId());
          _stateInfo.WaitChildProcId(NO_PROCESS_ID);
          setStatus(RUN);
        } else if(childProcess.value().status() == TERMINATED && childProcess.value().parentProcessID() == _processID) {
          childProcess.value().Release();
          removeChildProcessID(_stateInfo.WaitChildProcId());
          _stateInfo.WaitChildProcId(NO_PROCESS_ID);
          setStatus(RUN);
        }
      }
    }
    break;

    case WAIT_LOCK: {
      if(_stateInfo.IsWaitOnLockCompleted()) {
        setStatus(RUN);
      }
    }
    break;

    case WAIT_QUEUE: {
      auto& q = ProcessManager::Instance().getWaitQueue(_stateInfo.WaitQueueSpaceId(), _stateInfo.WaitQueueId());
      if (upan::find(q.begin(), q.end(), _processID) == q.end()) {
        _stateInfo.WaitQueueId(0);
        _stateInfo.WaitQueueSpaceId(NO_PROCESS_ID);
        setStatus(RUN);
      } else {
        if (_stateInfo.SleepTime() && PIT::Instance().GetClockCount() >= _stateInfo.SleepTime()) {
          _stateInfo.SleepTime(0);
          _stateInfo.setError(ProcessStateInfo::TIMEOUT);
          setStatus(RUN);
        } else if (interruptedBySignal) {
          _stateInfo.SleepTime(0);
          _stateInfo.setError(ProcessStateInfo::INTERRUPTED);
          setStatus(RUN);
        }
      }
    }
    break;

    case WAIT_KERNEL_SERVICE: {
      if(_stateInfo.IsKernelServiceComplete()) {
        _stateInfo.KernelServiceComplete(false);
        setStatus(RUN);
      }
    }
    break;

    case STOPPED:
    case RUN:
      break;
  }
}

void SchedulableProcess::applyDefaultSignalAction(const Signal& signal) {
  switch(signal.defaultActionType()) {
    case Signal::SA_TERMINATE: {
      if (isCoreProcess()) {
        KLog::warn("Can't terminate core process: %s (%d)", _name.c_str(), _processID);
      } else {
        Destroy();
      }
    }
    break;

    case Signal::SA_IGNORE:
      syslog(LOG_INFO, "Ignoring signal %d sent to process: %d", signal.signo(), _processID);
      break;

    case Signal::SA_STOP:
      setStatus(STOPPED);
      break;

    case Signal::SA_CONTINUE:
      setStatus(RUN);
      break;
  }
}

void SchedulableProcess::deliverPendingSignal() {
  if (_status == TERMINATED || _status == RELEASED) {
    return;
  }

  auto signalOpt = getSignal();
  if (signalOpt.isEmpty()) {
    return;
  }
  const auto& signal = signalOpt.value();

  if (signal.isMaskable()) {
    if (!sigismember(&_sigMask, signal.signo())) {
      auto handlerOpt = getSignalAction(signal.signo());
      if (handlerOpt.isEmpty() || isdefaultaction(&handlerOpt.value())) {
        applyDefaultSignalAction(signal);
      } else {
        auto& handler = handlerOpt.value();
        if (isignoreaction(&handler) || (_status == STOPPED && signal.signo() != SIGCONT)) {
          return;
        }

        try {
          setupSignalStackFrame(handler, signal);
          _sigMask = handler.sa_mask;
          sigaddset(&_sigMask, signal.signo());
          setStatus(RUN);
        } catch (upan::exception& e) {
          KLog::critical("Signal delivery failed for process: %d. Reason: %s", _processID, e.ErrorMsg().c_str());
          Destroy();
        }
      }
    }
  } else {
    applyDefaultSignalAction(signal);
  }
}