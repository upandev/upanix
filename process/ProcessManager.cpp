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
#include <MountManager.h>
#include <StringUtil.h>
#include <ProcessConstants.h>
#include <KernelUtil.h>
#include <exception.h>
#include <mutex.h>
#include <uniq_ptr.h>
#include <syscalldefs.h>
#include <KernelProcess.h>
#include <UserThread.h>
#include <Cpu.h>
#include <KernelRootProcess.h>

int ProcessManager::_currentProcessID = NO_PROCESS_ID;
int ProcessManager::_upanixKernelProcessID = NO_PROCESS_ID;
uint32_t ProcessManager::_taskSwitch = 0;
bool ProcessManager::_contextSwitch = false;

ProcessManager::ProcessManager() {
  for (bool& i : _resourceList) {
    i = false;
  }

  _processSchedulerIt = _processSchedulerList.end();

  KC::MConsole().LoadMessage("recv Manager Initialization", Success);
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

void ProcessManager::PrepareToRun(SchedulableProcess& process) {
  _currentProcessID = process.processID();
  ProcessStateInfo& stateInfo = process.stateInfo();

	switch(process.status()) {
	  case RELEASED:
      break;

	  case TERMINATED:
	    if (process.parentProcessID() == UpanixKernelProcessID()) {
	      process.Release();
	    }
      break;

  	case WAIT_SLEEP:
		{
      if(PIT::Instance().GetClockCount() >= stateInfo.SleepTime())
			{
        stateInfo.SleepTime(0) ;
				process.setStatus(RUN);
			}
			else
			{
				if(false)
				{
          printf("\n Sleep Time: %u", stateInfo.SleepTime()) ;
					printf("\n PIT Tick Count: %u", PIT::Instance().GetClockCount()) ;
					printf("\n") ;
				}
			}
		}
		break ;

	  case WAIT_INT:
		{
			if (WakeupProcessOnInterrupt(process)) {
			  process.setStatus(RUN);
			}
		}
		break ;

    case WAIT_INT_WITH_TIMEOUT:
    {
      if (WakeupProcessOnInterrupt(process)) {
        process.setStatus(RUN);
      } else {
        if (PIT::Instance().GetClockCount() >= stateInfo.SleepTime()) {
          stateInfo.SleepTime(0);
          process.setStatus(RUN);
        }
      }
    }
    break ;

    case WAIT_EVENT:
    {
      if(IsEventCompleted(process.processID())) {
        process.setStatus(RUN);
      }
    }
    break;

    case WAIT_IO_DESCRIPTORS:
	  {
	    const auto& result = process.iodTable().selectCheck(process.stateInfo().GetIODescriptors());
	    if (!result.empty()) {
        process.stateInfo().SetIODescriptors(result);
        process.stateInfo().setError(ProcessStateInfo::NO_ERROR);
        process.setStatus(RUN);
      } else {
        if (stateInfo.SleepTime() && PIT::Instance().GetClockCount() >= stateInfo.SleepTime()) {
          process.stateInfo().SleepTime(0);
          process.stateInfo().setError(ProcessStateInfo::TIMEOUT);
          process.setStatus(RUN);
        }
      }
	  }
	  break;

	  case WAIT_CHILD:
		{
      if(stateInfo.WaitChildProcId() < 0) {
        stateInfo.WaitChildProcId(NO_PROCESS_ID);
				process.setStatus(RUN);
			} else {
			  auto childProcess = GetSchedulableProcess(stateInfo.WaitChildProcId());
				if(childProcess.isEmpty() || childProcess.value().parentProcessID() != _currentProcessID) {
          process.removeChildProcessID(stateInfo.WaitChildProcId());
          stateInfo.WaitChildProcId(NO_PROCESS_ID);
					process.setStatus(RUN);
				} else if(childProcess.value().status() == TERMINATED && childProcess.value().parentProcessID() == _currentProcessID) {
          childProcess.value().Release();
          process.removeChildProcessID(stateInfo.WaitChildProcId());
          stateInfo.WaitChildProcId(NO_PROCESS_ID);
					process.setStatus(RUN);
				}
			}
		}
		break ;

    case WAIT_LOCK:
    {
      if(stateInfo.IsWaitOnLockCompleted()) {
        process.setStatus(RUN);
      }
    }
    break;

    case WAIT_QUEUE:
    {
      auto& q = _processWaitQueueMap[stateInfo.WaitQueueSpaceId()][stateInfo.WaitQueueId()];
      if (upan::find(q.begin(), q.end(), process.processID()) == q.end()) {
        stateInfo.WaitQueueId(0);
        stateInfo.WaitQueueSpaceId(NO_PROCESS_ID);
        process.setStatus(RUN);
      }
    }
    break;

	  case WAIT_RESOURCE:
		{
      if(stateInfo.WaitResourceId() == RESOURCE_NIL) {
				process.setStatus(RUN);
			} else {
        if(_resourceList[stateInfo.WaitResourceId()] == false) {
          stateInfo.WaitResourceId(RESOURCE_NIL);
					process.setStatus(RUN);
				}
			}
		}
		break ;

	  case WAIT_KERNEL_SERVICE:
		{
      if(stateInfo.IsKernelServiceComplete()) {
        stateInfo.KernelServiceComplete(false);
        process.setStatus(RUN);
			}
		}
		break ;

	  case RUN:
	    break ;
	}
}

void ProcessManager::ContextSwitch(TaskContext& taskContext) {
  Cpu::SetRegValue(Cpu::CR3, (uint64_t)MEM_PML4_TABLE);
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
      PrepareToRun(process);

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

void ProcessManager::WaitOnQueue(int id, upan::mutex &waitMutex, bool isKernelSpace) {
  if(GetCurProcId() < 0)
    return ;

  //isKernelSpace = true => condition_variable used in kernel code that includes syscall code that is executed by user processes/threads
  //In this space, the mutex and condition_variables are shared across processes/threads. Therefore, we need to use a global WaitQueueMap. The Space here is NO_PROCESS_ID => global
  //For condition_variables used withing a user process/thread, the mutex and condition_variables are shared within the process and its threads.
  //Therefore, the WaitQueue is local to that particular process. The space here is the PID of the main thread

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    const int spaceId = GetWaitQueueSpaceId(p, isKernelSpace);
    _processWaitQueueMap[spaceId][id].push_back(p.processID());
    p.stateInfo().WaitQueueId(id);
    p.stateInfo().WaitQueueSpaceId(spaceId);
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

void ProcessManager::WaitOnResource(RESOURCE_KEYS resourceKey)
{
	if(GetCurProcId() < 0)
		return ;

  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().WaitResourceId(resourceKey);
    p.setStatus(WAIT_RESOURCE);
  }
  p.yield();
}

void ProcessManager::WaitOnIODescriptor(int fd, IO_OP_TYPES waitType, time_t timeoutInMs) {
  upan::vector<io_descriptor> waitIODescriptors;
  io_descriptor waitIODescriptor;
  waitIODescriptor._fd = fd;
  waitIODescriptor._ioType = waitType;
  waitIODescriptors.push_back(waitIODescriptor);
  WaitOnIODescriptors(waitIODescriptors, timeoutInMs);
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
                                        byte bIsFGProcess, const upan::vector<uintptr_t>& params) {
  try {
    upan::uniq_ptr<SchedulableProcess> newPAS(new KernelProcess(name, uiTaskAddress, iParentProcessID, bIsFGProcess, params));
    int pid = newPAS->processID();
    AddToSchedulerList(*newPAS.release());
    return pid;
  } catch(upan::exception& ex) {
    ex.Print();
  }
	return -1;
}

int ProcessManager::Create(const upan::string& name, int iParentProcessID, byte bIsFGProcess, int iUserID, int iNumberOfParameters, char** szArgumentList) {
  try {
    upan::uniq_ptr<SchedulableProcess> newPAS(new UserProcess(name, iParentProcessID, iUserID, bIsFGProcess, iNumberOfParameters, szArgumentList));
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

PS* ProcessManager::GetProcList(unsigned& uiListSize)
{
  ProcessSwitchLock lock;
  uiListSize = _processMap.size();

  PS* pProcList;
  PS* pPS;
  Process& pAddrSpc = GetCurrentPAS() ;

  if(pAddrSpc.isKernelProcess())
  {
    pPS = pProcList = (PS*)KernelDMM::Instance().allocate(sizeof(PS) * uiListSize) ;
  }
  else
  {
    pProcList = (PS*)pAddrSpc.dmm().allocate(sizeof(PS) * uiListSize) ;
    pPS = (PS*)(pProcList);
  }

  auto it = _processMap.begin();
  for(int i = 0; it != _processMap.end(); ++i, ++it)
  {
    SchedulableProcess& p = *it->second;

    pPS[i].pid = p.processID();
    pPS[i].status = p.status() ;
    pPS[i].iParentProcessID = p.parentProcessID() ;
    pPS[i].iProcessGroupID = p.processGroup()->Id();
    pPS[i].iUserID = p.userID() ;

    char* pname ;
    if(pAddrSpc.isKernelProcess())
    {
      pname = pPS[i].pname = (char*)KernelDMM::Instance().allocate(p.name().length() + 1) ;
    }
    else
    {
      pPS[i].pname = (char*)pAddrSpc.dmm().allocate(p.name().length() + 1) ;
      pname = (char*)(pPS[i].pname);
    }
    strcpy(pname, p.name().c_str()) ;
  }

  return pProcList;
}

void ProcessManager::FreeProcListMem(PS* pProcList, unsigned uiListSize)
{
  Process& pAddrSpc = GetCurrentPAS();

	for(unsigned i = 0; i < uiListSize; i++)
	{
		if(pAddrSpc.isKernelProcess())
			KernelDMM::Instance().free((uintptr_t)pProcList[i].pname) ;
		else
			pAddrSpc.dmm().free((uintptr_t)pProcList[i].pname) ;
	}

	if(pAddrSpc.isKernelProcess())
		KernelDMM::Instance().free((uintptr_t)pProcList) ;
	else
	  pAddrSpc.dmm().free((uintptr_t)pProcList);
}

bool ProcessManager::IsDMMOn(int iProcessID) {
	return GetSchedulableProcess(iProcessID).value().dmm().isDmmFlag();
}

bool ProcessManager::IsKernelProcess(int iProcessID) {
	return GetSchedulableProcess(iProcessID).value().isKernelProcess();
}

void ProcessManager_Exit() {
  if (IsKernel()) {
    __asm__ __volatile__("HLT");
  }
  auto& p = ProcessManager::Instance().GetCurrentPAS();
  p.setStatus(TERMINATED);
  p.yield();
}

bool ProcessManager::IsResourceBusy(__volatile__ RESOURCE_KEYS uiType)
{
	return _resourceList[ uiType ] ;
}

void ProcessManager::SetResourceBusy(RESOURCE_KEYS uiType, bool bVal)
{
	_resourceList[ uiType ] = bVal ;
}

int ProcessManager::GetCurProcId()
{
	return IsKernel() ? NO_PROCESS_ID : ProcessManager::GetCurrentProcessID();
}

void ProcessManager::Kill(int iProcessID) {
  GetSchedulableProcess(iProcessID).ifPresent([this, iProcessID](SchedulableProcess& process) {
    if (process.status() != TERMINATED && process.status() != RELEASED) {
      if (iProcessID == GetCurProcId()) {
        process.setStatus(TERMINATED);
        process.yield();
      } else {
        process.Destroy();
      }
    }
  });
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

bool ProcessManager::WakeupProcessOnInterrupt(SchedulableProcess& p)
{
  const IRQ& irq = *p.stateInfo().Irq();

	if(irq == StdIRQ::Instance().NO_IRQ)
		return true ;

	return irq.Consume();
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
