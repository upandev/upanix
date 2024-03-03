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
#include <PortCom.h>
#include "KernelRootProcess.h"

int ProcessManager::_currentProcessID = NO_PROCESS_ID;
int ProcessManager::_upanixKernelProcessID = NO_PROCESS_ID;
uint32_t ProcessManager::_taskSwitch = 0;
bool ProcessManager::_contextSwitch = false;

ProcessManager::ProcessManager() {
  for (bool& i : _resourceList) {
    i = false;
  }

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
  if (IS_KERNEL()) {
    return KernelRootProcess::Instance();
  }
  //This function is a utility that assumes that a ProcessAddressSpace entry always exists for current (active) process
  //Caller should take care of cases when ProcessID = NO_PROCESS_ID - which is usually the case before kernel scheduler is initialized
  return GetSchedulableProcess(_currentProcessID).value();
}

void ProcessManager::BuildCallGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount)
{
	GateDescriptor* gateEntry = (GateDescriptor*)(/*GDT_BASE_ADDR + */usGateSelector) ;
	
//	__asm__ __volatile__("push %ds") ;
//	MemUtil_SetDS(SYS_LINEAR_SELECTOR_DEFINED) ;

	gateEntry->lowerOffset = uiOffset & 0x0000FFFF ;
	gateEntry->selector = usSelector ;
	gateEntry->parameterCount = bParameterCount & 0x1F ;
	gateEntry->options = 0xEC ; // 11101100 --> 1 = Present. 11 = DPL. 01100 = CallGateType 
	gateEntry->upperOffset = (uiOffset & 0xFFFF0000) >> 16 ;

//	__asm__ __volatile__("pop %ds") ;
}

void ProcessManager::BuildIntGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount)
{
	GateDescriptor* gateEntry = (GateDescriptor*)(/*GDT_BASE_ADDR + */usGateSelector) ;
	
	//__asm__ __volatile__("push %ds") ;
//	MemUtil_SetDS(SYS_LINEAR_SELECTOR_DEFINED) ;

	gateEntry->lowerOffset = uiOffset & 0x0000FFFF ;
	gateEntry->selector = usSelector ;
	gateEntry->parameterCount = bParameterCount & 0x1F ;
	gateEntry->options = 0xEE ; // 11101110 --> 1 = Present. 11 = DPL. 01110 = IntGateType 
	
	gateEntry->upperOffset = (uiOffset & 0xFFFF0000) >> 16 ;

	//__asm__ __volatile__("pop %ds") ;
}

void ProcessManager::BuildIntTaskState(const unsigned uiTaskAddress, const unsigned uiTSSAddress, const int stack)
{
	TaskState* taskState = (TaskState*)(uiTSSAddress - GLOBAL_DATA_SEGMENT_BASE) ;
	memset(taskState, 0, sizeof(TaskState)) ;

	taskState->EIP = uiTaskAddress ;
	taskState->ESP = stack ;

	taskState->ES = 0x8 ;

	taskState->CS = 0x10 ;
	taskState->DS = 0x18 ;
	taskState->SS = 0x18 ;
	taskState->FS = 0x18 ;
	taskState->GS = 0x18 ;
	taskState->LDT = 0x0 ;

	//taskState->CR3_PDBR = MEM_PML4 ;
	taskState->EFLAGS = 0x202 ;
	taskState->DEBUG_T_BIT = 0x00 ;
	taskState->IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
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
			if (!WakeupProcessOnInterrupt(process)) {
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
        process.setStatus(RUN);
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

void ProcessManager::Sleep(__volatile__ unsigned sleepTime) // in Milli Seconds
{
	if(DoPollWait()) {
		KernelUtil::Wait(sleepTime) ;
		return ;
	}

  auto &p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    //ProcessManager::DisableTaskSwitch() ;
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

void ProcessManager::WaitOnIODescriptor(int fd, IO_OP_TYPES waitType) {
  upan::vector<io_descriptor> waitIODescriptors;
  io_descriptor waitIODescriptor;
  waitIODescriptor._fd = fd;
  waitIODescriptor._ioType = waitType;
  waitIODescriptors.push_back(waitIODescriptor);
  WaitOnIODescriptors(waitIODescriptors);
}

void ProcessManager::WaitOnIODescriptors(const upan::vector<io_descriptor>& waitIODescriptors) {
  if(GetCurProcId() < 0)
    return ;
  auto& p = GetCurrentPAS();
  {
    ProcessSwitchLock lock;
    p.stateInfo().SetIODescriptors(waitIODescriptors);
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
    //MemManager::Instance().DisplayNoOfFreePages() ;
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
    //MemManager::Instance().DisplayNoOfFreePages() ;
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
    pPS = pProcList = (PS*)DMM_AllocateForKernel(sizeof(PS) * uiListSize) ;
  }
  else
  {
    pProcList = (PS*)DMM_Allocate(&pAddrSpc, sizeof(PS) * uiListSize) ;
    pPS = (PS*)PROCESS_REAL_ALLOCATED_ADDRESS(pProcList);
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
      pname = pPS[i].pname = (char*)DMM_AllocateForKernel(p.name().length() + 1) ;
    }
    else
    {
      pPS[i].pname = (char*)DMM_Allocate(&pAddrSpc, p.name().length() + 1) ;
      pname = (char*)PROCESS_REAL_ALLOCATED_ADDRESS(pPS[i].pname);
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
			DMM_DeAllocateForKernel((uintptr_t)pProcList[i].pname) ;
		else
			DMM_DeAllocate(&pAddrSpc, (uintptr_t)pProcList[i].pname) ;
	}

	if(pAddrSpc.isKernelProcess())
		DMM_DeAllocateForKernel((uintptr_t)pProcList) ;
	else
		DMM_DeAllocate(&pAddrSpc, PROCESS_VIRTUAL_ALLOCATED_ADDRESS(pProcList));
}

void ProcessManager::SetDMMFlag(int iProcessID, bool flag) {
  GetSchedulableProcess(iProcessID).value().setDmmFlag(flag);
}

bool ProcessManager::IsDMMOn(int iProcessID) {
	return GetSchedulableProcess(iProcessID).value().isDmmFlag();
}

bool ProcessManager::IsKernelProcess(int iProcessID) {
	return GetSchedulableProcess(iProcessID).value().isKernelProcess();
}

void ProcessManager_Exit() {
  if (IS_KERNEL()) {
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
	return KERNEL_MODE ? NO_PROCESS_ID : ProcessManager::GetCurrentProcessID();
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

bool ProcessManager::CopyDiskDrive(int iProcessID, int& iOldDriveId, FileSystem::PresentWorkingDirectory& mOldPWD) {
	if(GetCurProcId() < 0)
		return false;

  SchedulableProcess* pSrcPAS = &GetSchedulableProcess(iProcessID).value();
  Process* pDestPAS = &GetCurrentPAS();

	iOldDriveId = pDestPAS->driveID() ;
  memcpy(&mOldPWD, &(pDestPAS->processPWD()), sizeof(FileSystem::PresentWorkingDirectory));

  pDestPAS->setDriveID(pSrcPAS->driveID());
  memcpy(&(pDestPAS->processPWD()), &(pSrcPAS->processPWD()), sizeof(FileSystem::PresentWorkingDirectory));

	return true;
}

bool ProcessManager::WakeupProcessOnInterrupt(SchedulableProcess& p)
{
  const IRQ& irq = *p.stateInfo().Irq();

	if(irq == StdIRQ::Instance().NO_IRQ)
		return true ;

	return irq.Consume();
}

bool ProcessManager::DoPollWait() {
	return (KERNEL_MODE || !IsTaskSwitchEnabled()) ;
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
