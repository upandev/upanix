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
#include <ProcessManager.h>
#include <FileSystem.h>
#include <IrqManager.h>
#include <PIT.h>
#include <DMM.h>
#include <KernelService.h>
#include <UserManager.h>
#include <StringUtil.h>
#include <ProcessConstants.h>
#include <KernelUtil.h>
#include <exception.h>
#include <mutex.h>
#include <uniq_ptr.h>
#include <syscalldefs.h>
#include <KernelProcess.h>
#include <UserThread.h>
#include <KernelRootProcess.h>

int ProcessManager::_currentProcessID = NO_PROCESS_ID;
int ProcessManager::_upanixKernelProcessID = NO_PROCESS_ID;
uint32_t ProcessManager::_taskSwitch = 0;
bool ProcessManager::_contextSwitch = false;

ProcessManager::ProcessManager() {
  _processSchedulerIt = _processSchedulerList.end();
  KC::MConsole().LoadMessage("Process Manager Initialization", Success);
}

AutonomousProcess& ProcessManager::GetThreadParentProcess(int pid) {
  ProcessSwitchLock switchLock;
  auto parent = GetSchedulableProcess(pid);
  if (parent.isEmpty()) {
    throw upan::exception(XLOC, "No parent process found for pid: %d", pid);
  }

  auto autonomousProcess = dynamic_cast<AutonomousProcess*>(&parent.value());
  if (autonomousProcess) {
    return *autonomousProcess;
  }

  auto thread = dynamic_cast<Thread*>(&parent.value());
  if (thread) {
    return thread->threadParent();
  }

  throw upan::exception(XLOC, "parent of a thread can be either a AutonomousProcess or another Thread");
}

upan::option<Process&> ProcessManager::GetProcess(int pid) {
  if (pid == NO_PROCESS_ID) {
    return upan::option<Process&>(KernelRootProcess::Instance());
  }
  return GetSchedulableProcess(pid).map<Process&>([](SchedulableProcess& p) -> Process& { return p; });
}

upan::option<SchedulableProcess&> ProcessManager::GetSchedulableProcess(int pid) {
  ProcessSwitchLock switchLock;
  auto it = _processMap.find(pid);
  if (it == _processMap.end()) {
    return upan::option<SchedulableProcess&>::empty();
  }
  return upan::option<SchedulableProcess&>(*it->second);
}

Process& ProcessManager::GetCurrentPAS() {
  ProcessSwitchLock switchLock;
  if (IsKernel()) {
    return KernelRootProcess::Instance();
  }
  //This function is a utility that assumes that a ProcessAddressSpace entry always exists for current (active) process
  //Caller should take care of cases when ProcessID = NO_PROCESS_ID - which is usually the case before kernel scheduler is initialized
  return GetSchedulableProcess(_currentProcessID).value();
}

void ProcessManager::AddToSchedulerList(SchedulableProcess& process) {
  IrqGuard g;
  AddToProcessMap(process);
  process.setStatus(RUN);
  _processSchedulerList.push_back(&process);
}

void ProcessManager::AddToProcessMap(SchedulableProcess& process) {
  ProcessSwitchLock switchLock;
  if(_processMap.size() + 1 > MAX_NO_PROCESS)
    throw upan::exception(XLOC, "Max process limit reached!");
  _processMap.insert(ProcessMap::value_type(process.processID(), &process));
}

void ProcessManager::RemoveFromProcessMap(SchedulableProcess& process) {
  ProcessSwitchLock switchLock;
  _processMap.erase(process.processID());
}

void ProcessManager::ContextSwitch(TaskContext& taskContext) {
  KernelRootProcess::Instance().switchPageTable();
  const auto& p = GetSchedulableProcess(GetCurrentProcessID());
  if (!p.isEmpty()) {
    auto &currentProcess = p.value();
    if (currentProcess.status() == PROCESS_STATUS::RUN && (!IsTaskSwitchEnabled() || !currentProcess.CanPreempt())) {
      currentProcess.switchPageTable();
      return;
    } else if (currentProcess.status() == TERMINATED) {
      currentProcess.Destroy();
    } else {
      currentProcess.Store(taskContext);
    }
    EnableTaskSwitch();
  }

  if (IsTaskSwitchEnabled()) {
    while (!_processSchedulerList.empty()) {
      if (_processSchedulerIt == _processSchedulerList.end()) {
        _processSchedulerIt = _processSchedulerList.begin();
      }

      auto &process = (*_processSchedulerIt)->forSchedule();
      _currentProcessID = process.processID();
      process.prepareToRun();
      process.deliverPendingSignal();

      if (process.status() == PROCESS_STATUS::RUN) {
        process.Load(taskContext);
        process.switchPageTable();
        ++_processSchedulerIt;
        break;
      } else if (!process.isChildThread() && process.status() == RELEASED) {
        _processSchedulerList.erase(_processSchedulerIt++);
        RemoveFromProcessMap(process);
        delete &process;
      } else {
        ++_processSchedulerIt;
      }
    }
  }
}

//return true if it was previously disabled and now enabled
bool ProcessManager::EnableTaskSwitch() {
  return upan::atomic::op::swap(_taskSwitch, 1) == 0;
}

//return true if it was previously enabled and now disabled
bool ProcessManager::DisableTaskSwitch() {
  return upan::atomic::op::swap(_taskSwitch, 0) == 1;
}

void ProcessManager::Sleep(uint32_t sleepTime) // in Milli Seconds
{
	if(DoPollWait()) {
		KernelUtil::Wait(sleepTime) ;
		return ;
	}

  auto &p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().SleepTime(PIT::Instance().GetClockCount() + PIT::Instance().RoundSleepTime(sleepTime));
    p.setStatus(WAIT_SLEEP);
  }
  p.yield();
}

void ProcessManager::WaitOnInterrupt(const IRQ& irq)
{
	if(DoPollWait())
	{
		KernelUtil::WaitOnInterrupt(irq);
		return;
	}

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().Irq(&irq);
    p.setStatus(WAIT_INT);
  }
  p.yield();
}

void ProcessManager::WaitOnInterruptWithTimeout(const IRQ& irq, uint32_t timeout)
{
  if(DoPollWait())
  {
    KernelUtil::WaitOnInterrupt(irq);
    return;
  }

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().Irq(&irq);
    p.stateInfo().SleepTime(PIT::Instance().GetClockCount() + PIT::Instance().RoundSleepTime(timeout));
    p.setStatus(WAIT_INT_WITH_TIMEOUT);
  }
  p.yield();
}

void ProcessManager::WaitForEvent()
{
  if(DoPollWait())
  {
    while(!IsEventCompleted(GetCurProcId()))
    {
      __asm__ __volatile__("nop") ;
      __asm__ __volatile__("nop") ;
    }
    return;
  }

  auto& p = GetCurrentPAS();
  p.setStatus(WAIT_EVENT);
  p.yield();
}

void ProcessManager::WaitOnChild(int iChildProcessID)
{
	if(GetCurProcId() < 0)
		return ;

	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
		return ;

	auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().WaitChildProcId(iChildProcessID);
    p.setStatus(WAIT_CHILD);
  }
  p.yield();
}

void ProcessManager::WaitOnLock(upan::atomic::integral<int>* waitLock, int oldVal, int newVal) {
  if(GetCurProcId() < 0)
    return ;

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    upan::atomic::integral<int>* covertWaitLock = waitLock;
    if (!p.isKernelProcess()) {
      covertWaitLock = (upan::atomic::integral<int>*)MemManager::Instance().GetFlatAddress(p.pml4Table(), (uint64_t)waitLock);
    }
    p.stateInfo().WaitOnLock(covertWaitLock, oldVal, newVal);
    p.setStatus(WAIT_LOCK);
  }
  p.yield();
}

int ProcessManager::GetWaitQueueSpaceId(Process& process, bool isKernelSpace) {
  return isKernelSpace ? NO_PROCESS_ID : dynamic_cast<SchedulableProcess&>(process).mainThreadID();
}

void ProcessManager::WaitOnQueue(int id, upan::mutex &waitMutex, time_t timeoutInMs, bool isKernelSpace) {
  if(GetCurProcId() < 0)
    return ;

  //isKernelSpace = true => condition_variable used in kernel code that includes syscall code that is executed by user processes/threads
  //In this space, the mutex and condition_variables are shared across processes/threads. Therefore, we need to use a global WaitQueueMap. The Space here is NO_PROCESS_ID => global
  //For condition_variables used within a user process/thread, the mutex and condition_variables are shared within the process and its threads.
  //Therefore, the WaitQueue is local to that particular process. The space here is the PID of the main thread

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    const int spaceId = GetWaitQueueSpaceId(p, isKernelSpace);
    _processWaitQueueMap[spaceId][id].push_back(p.processID());
    p.stateInfo().WaitQueueId(id);
    p.stateInfo().WaitQueueSpaceId(spaceId);
    if (timeoutInMs) {
      p.stateInfo().SleepTime(PIT::Instance().GetClockCount() + PIT::Instance().RoundSleepTime(timeoutInMs));
    } else {
      p.stateInfo().SleepTime(0);
    }
    p.setStatus(WAIT_QUEUE);
    waitMutex.unlock();
  }
  p.yield();
}

void ProcessManager::WaitDequeue(int id, bool all, bool isKernelSpace) {
  if(GetCurProcId() < 0)
    return ;

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    const int spaceId = GetWaitQueueSpaceId(p, isKernelSpace);
    auto& wq = _processWaitQueueMap[spaceId][id];
    if (all) { wq.clear(); }
    else { wq.pop_front(); }
  }
}

void ProcessManager::WaitOnIODescriptor(int fd, IO_OP_TYPES waitType, time_t timeoutInMs) {
  upan::vector<io_descriptor> waitIODescriptors;
  io_descriptor waitIODescriptor;
  waitIODescriptor._fd = fd;
  waitIODescriptor._ioType = waitType;
  waitIODescriptors.push_back(waitIODescriptor);
  WaitOnIODescriptors(waitIODescriptors, timeoutInMs);
}

void ProcessManager::WaitOnTerminalIO(const upan::string& path, FSTerminalDevice::TERMINAL_IO_TYPES waitType, time_t timeoutInMs) {
  if(GetCurProcId() < 0)
    return ;
  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    FSTerminalDevice::WaitInfo waitInfo;
    waitInfo._path = path;
    waitInfo._waitType = waitType;
    p.stateInfo().SetTerminalIOWaitInfo(waitInfo);
    p.stateInfo().setError(ProcessStateInfo::NO_ERROR);
    if (timeoutInMs) {
      p.stateInfo().SleepTime(PIT::Instance().GetClockCount() + PIT::Instance().RoundSleepTime(timeoutInMs));
    } else {
      p.stateInfo().SleepTime(0);
    }
    p.setStatus(WAIT_TERMINAL_IO);
  }
  p.yield();
}

void ProcessManager::WaitOnIODescriptors(const upan::vector<io_descriptor>& waitIODescriptors, time_t timeoutInMs) {
  if(GetCurProcId() < 0)
    return ;
  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().SetIODescriptors(waitIODescriptors);
    p.stateInfo().setError(ProcessStateInfo::NO_ERROR);
    if (timeoutInMs) {
      p.stateInfo().SleepTime(PIT::Instance().GetClockCount() + PIT::Instance().RoundSleepTime(timeoutInMs));
    } else {
      p.stateInfo().SleepTime(0);
    }
    p.setStatus(WAIT_IO_DESCRIPTORS);
  }
  p.yield();
}

bool ProcessManager::IsAlive(int pid) {
  auto process = GetSchedulableProcess(pid);
  return !process.isEmpty() && process.value().status() != PROCESS_STATUS::TERMINATED && process.value().status() != PROCESS_STATUS::RELEASED;
}

bool ProcessManager::IsChildAlive(int iChildProcessID) {
	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
    return false;
	auto process = GetSchedulableProcess(iChildProcessID);
  return !process.isEmpty() && process.value().parentProcessID() == ProcessManager::GetCurrentProcessID();
}

int ProcessManager::CreateKernelProcess(const upan::string& name, const uintptr_t uiTaskAddress, int iParentProcessID,
                                        bool isFGProcess, bool isCoreProcess, const upan::vector<uintptr_t>& params) {
  try {
    upan::uniq_ptr<SchedulableProcess> newPAS(new KernelProcess(name, uiTaskAddress, iParentProcessID, isFGProcess, isCoreProcess, params));
    int pid = newPAS->processID();
    AddToSchedulerList(*newPAS.release());
    return pid;
  } catch(upan::exception& ex) {
    ex.Print();
  }
	return -1;
}

int ProcessManager::Create(const upan::string& name, int iParentProcessID, byte bIsFGProcess, int iUserID,
                           const upan::vector<upan::string>& argv,
                           const upan::vector<upan::string>& envp) {
  try {
    upan::uniq_ptr<SchedulableProcess> newPAS(new UserProcess(name, iParentProcessID, iUserID, bIsFGProcess, argv, envp));
    int pid = newPAS->processID();
    AddToSchedulerList(*newPAS.release());
    return pid;
  }
  catch(const upan::exception& e) {
    e.Print();
  }
  return -1;
}

//TODO:
//1: Lock Env Page access
//2: Lock FileDescriptor Table access
//3: Lock process heap access
//4: DLL service
int ProcessManager::CreateThreadTask(int parentID, uintptr_t threadCaller, uintptr_t threadEntryAddress, void* arg) {
  try {
    AutonomousProcess& parent = ProcessManager::Instance().GetThreadParentProcess(parentID);
    upan::uniq_ptr<SchedulableProcess> threadPAS(&parent.CreateThread(threadCaller, threadEntryAddress, arg));
    int threadID = threadPAS->processID();
    AddToProcessMap(*threadPAS.release());
    return threadID;
  } catch(const upan::exception& e) {
    e.Print();
  }
  return -1;
}

PS* ProcessManager::GetProcList(unsigned& uiListSize) {
  ProcessSwitchLock lock;
  uiListSize = _processMap.size();

  PS* procList;
  Process& process = GetCurrentPAS() ;

  procList = (PS*)process.dmm().allocate(sizeof(PS) * uiListSize);

  auto it = _processMap.begin();
  for(int i = 0; it != _processMap.end(); ++i, ++it) {
    SchedulableProcess& p = *it->second;

    procList[i].pid = p.processID();
    procList[i].status = p.status() ;
    procList[i].iParentProcessID = p.parentProcessID() ;
    procList[i].iProcessGroupID = p.processGroup()->Id();
    procList[i].iUserID = p.userID() ;

    procList[i].pname = (char*)process.dmm().allocate(p.name().length() + 1);
    strcpy(procList[i].pname, p.name().c_str()) ;
  }

  return procList;
}

void ProcessManager::FreeProcListMem(PS* procList, unsigned uiListSize) {
  Process& process = GetCurrentPAS();

	for(auto i = 0; i < uiListSize; i++) {
    process.dmm().free((uintptr_t)procList[i].pname) ;
	}

  process.dmm().free((uintptr_t)procList);
}

bool ProcessManager::IsDMMOn(int iProcessID) {
	return GetSchedulableProcess(iProcessID).value().dmm().isDmmFlag();
}

bool ProcessManager::IsKernelProcess(int iProcessID) {
	return GetSchedulableProcess(iProcessID).value().isKernelProcess();
}

void ProcessManager_Exit(int exitStatus) {
  if (IsKernel()) {
    __asm__ __volatile__("HLT");
  }
  auto& p = ProcessManager::Instance().GetCurrentPAS();
  p.setStatus(TERMINATED);
  p.stateInfo().setExitStatus(exitStatus);
  p.yield();
}

int ProcessManager::GetCurProcId()
{
	return IsKernel() ? NO_PROCESS_ID : ProcessManager::GetCurrentProcessID();
}

void ProcessManager::WakeUpFromKSWait(int iProcessID) {
  GetSchedulableProcess(iProcessID).ifPresent([](SchedulableProcess& process) {
    process.stateInfo().KernelServiceComplete(true);
  });
}

void ProcessManager::WaitOnKernelService() {
	if(GetCurProcId() < 0)
		return ; 

	auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    if (!p.stateInfo().IsKernelServiceComplete()) {
      p.stateInfo().KernelServiceComplete(false);
    }
    p.setStatus(WAIT_KERNEL_SERVICE);
  }
  p.yield();
}

bool ProcessManager::DoPollWait() {
	return (IsKernel() || !IsTaskSwitchEnabled()) ;
}

bool ProcessManager::ConditionalWait(const volatile unsigned* registry, unsigned bitPos, bool waitfor)
{
	if(bitPos > 31 || bitPos < 0)
		return false ;
  unsigned value = 1 << bitPos;

	int iMaxLimit = 1000 ; // 1 Sec
	unsigned uiSleepTime = 10 ; // 10 ms

	while(iMaxLimit > 10)
	{
    const bool res = ((*registry) & value) ? true : false;
    if(res == waitfor)
      return true;
		Sleep(uiSleepTime) ;
		iMaxLimit -= uiSleepTime ;
	}

	return false ;
}

bool ProcessManager::IsEventCompleted(int pid) {
  return GetProcessStateInfo(pid).IsEventCompleted();
}

void ProcessManager::EventCompleted(int pid) {
  GetProcessStateInfo(pid).EventCompleted();
}

ProcessStateInfo& ProcessManager::GetProcessStateInfo(int pid) {
  return GetSchedulableProcess(pid).map<ProcessStateInfo&>(
      [](SchedulableProcess& p) -> ProcessStateInfo& { return p.stateInfo(); })
      .valueOrElse(_kernelModeStateInfo);
}

void ProcessManager::closeAllFiles(StorageDrive& storageDrive) {
  ProcessSwitchLock lock;
  for (auto& process : _processMap) {
    process.second->iodTable().closeAllFiles(storageDrive);
  }
}

void ProcessManager::updateAllIODescriptorRedirections(pid_t pid, int srcFD, IODescriptor::Ptr targetDesc) {
  ProcessSwitchLock lock;
  Process& process = GetProcess(pid).valueOrThrow(XLOC, upan::error("failed to find process with pid %d", pid).Msg());
  process.iodTable().updateRedirections(srcFD, targetDesc);
  if (pid == NO_PROCESS_ID) {
    for (auto& p : _processMap) {
      if (!p.second->isChildThread()) {
        p.second->iodTable().updateRedirections(srcFD, targetDesc);
      }
    }
  }
}

void ProcessManager::MaskSignal(SIG_MASKING_TYPE how, const sigset_t *set, sigset_t *oldset) {
  ProcessSwitchLock lock;
  GetSchedulableProcess(_currentProcessID).ifPresent([&](SchedulableProcess& process) { process.maskSignal(how, set, oldset); });
}

void ProcessManager::SendSignal(pid_t pid, SIGNAL signo, const union sigval* value) {
  ProcessSwitchLock lock;
  GetSchedulableProcess(pid).ifPresent([&signo, &value](SchedulableProcess& process) { process.queueSignal(signo, value); });
}

void ProcessManager::SetSignalAction(SIGNAL signo, const struct sigaction* newact, struct sigaction* oldact) {
  ProcessSwitchLock pLock;
  GetSchedulableProcess(_currentProcessID).ifPresent([&](SchedulableProcess& process) {
    process.setSignalAction(signo, newact, oldact);
  });
}

void ProcessManager::SignalReturn(SignalTaskContext& signalTaskContext) {
  //don't put this under process-switch lock because the process will resume on a completely different RIP after signal return
  GetSchedulableProcess(_currentProcessID).ifPresent([&](SchedulableProcess& process) {
    process.setSignalRestoreContext(signalTaskContext);
    process.setStatus(SIGNAL_RETURN);
    process.yield();
  });
}

void ProcessManager::stopProcesses(upan::function<bool, SchedulableProcess&> stopCondition) {
  {
    ProcessSwitchLock lock;
    for (auto& e : _processMap) {
      auto& process = *e.second;
      if (stopCondition(process)) {
        printf("\n sending SIGTERM to %s (%d)", process.name().c_str(), process.processID());
        SendSignal(process.processID(), SIGTERM, nullptr);
      }
    }
  }

  printf("\n waiting for all processes to terminate...");
  sleepms(100);
  for (int i = 0; i < 20; ++i) {
    {
      ProcessSwitchLock lock;
      for (auto& e : _processMap) {
        auto& process = *e.second;
        if (stopCondition(process)) {
          sleepms(100);
          continue;
        }
      }
    }
  }

  {
    ProcessSwitchLock lock;
    for (auto& e : _processMap) {
      auto& process = *e.second;
      if (stopCondition(process)) {
        printf("\n force terminating %s (%d)", process.name().c_str(), process.processID());
        process.setStatus(TERMINATED);
        process.yield();
      }
    }
  }
}

void ProcessManager::stopUserProcesses() {
  stopProcesses([](SchedulableProcess& process) { return !process.isKernelProcess() && !process.isChildThread(); });
}

void ProcessManager::stopKernelProcesses() {
  stopProcesses([](SchedulableProcess& process) { return process.isKernelProcess() && !process.isCoreProcess() && !process.isChildThread(); });
}

