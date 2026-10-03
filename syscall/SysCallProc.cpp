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
# include <SysCall.h>
# include <SysCallDisplay.h>
# include <typeinfo.h>
# include <TscClock.h>

bool SysCallProc_IsPresent(uint64_t sysCallId) {
	return (sysCallId > SYS_CALL_PROC_START && sysCallId < SYS_CALL_PROC_END);
}

void SysCallProc_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5) {
	switch(sysCallId)
	{
		case SYS_CALL_DLL_RELOCATE :
			// P8 => Second Entry in GOT
			// P9 => Relocation Table Offset
      try
      {
        //ProcessManager_DisableTaskSwitch() ;
        DynamicLinkLoader_DoRelocation(ProcessManager::Instance().GetCurrentPAS(), (int)p4, p5, retVal);
        //ProcessManager_EnableTaskSwitch() ;
      }
      catch(const upan::exception& e)
      {
        printf("\n Dynamic Relocation Failed: %s", e.ErrorMsg().c_str());
      }
			break ;

    case SYS_CALL_PROCESS_INIT_RELOCATE:
      try {
        auto& process = ProcessManager::Instance().GetCurrentPAS();
        if (typeid(process) != typeid(UserProcess)) {
          throw upan::exception(XLOC, "DLL Init Relocate can be done on a User process once at program start-up");
        }
        *retVal = (uintptr_t) dynamic_cast<UserProcess&>(process).initRelocate();
      } catch(const upan::exception& e) {
        printf("\n Dynamic Init Relocation Failed: %s", e.ErrorMsg().c_str());
      }
      break;

    case SYS_CALL_PROCESS_FORK: {
      if (iskernel()) {
        KLog::error("kernel can't fork a process");
        *retVal = -1;
      }
      try {
        *retVal = KC::MKernelService().RequestFork();
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

		case SYS_CALL_PROCESS_EXEC :
			// P1 => Address of File Name Char Array
			// P2 => Number Of Args
			// P3 => Arg Vector Address
			{
				//ProcessManager_DisableTaskSwitch() ;
				
				auto szFile = (char*) p1;
				auto argv = (const char**)p2;
        auto envp = (const char**)p3;

				*retVal = KC::MKernelService().RequestProcessExec(szFile, argv, envp);

				//ProcessManager_EnableTaskSwitch() ;
			}
			break ;

	  case SYS_CALL_THREAD_EXEC :
	    // P1 => thread caller
      // P2 => thread entry point
	    // P3 => thread (input) arg
      // P4 => joinable
      {
        *retVal = KC::MKernelService().RequestThreadExec(p1, p2, (void*) p3, (bool)p4);
      }
      break;

		case SYS_CALL_PROCESS_WAIT_PID:
			// P1 => PID
			{
        int status;
				*retVal = ProcessManager::Instance().WaitOnChild((int)p1, status);
        auto exitStatus = (int*)p2;
        if (exitStatus) {
          *exitStatus = status;
        }
			}
			break;

    case SYS_CALL_PROCESS_WAIT_ON_LOCK:
      // P1 => Atomic Lock Address
      // P2 => new value
      // P3 => current value
      {
        auto lock = (upan::atomic::integral<int>*)p1;
        ProcessManager::Instance().WaitOnLock(lock, (int)p2, (int)p3) ;
      }
      break;

    case SYS_CALL_PROCESS_WAIT_QUEUE:
      // P1 => queue id
      // P2 => mutex address
      // P3 => timeout
      {
        const auto timeout = (struct timeval*)p3;
        time_t timeoutInMicroSeconds = 0;
        if (timeout) {
          timeoutInMicroSeconds = timeout->tv_sec * 1000000 + timeout->tv_usec;
        }
        *retVal = 0;
        ProcessManager::Instance().WaitOnQueue((int) p1, *reinterpret_cast<upan::mutex *>(p2), timeoutInMicroSeconds, false);
        const auto r = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
        if (r != ProcessStateInfo::NO_ERROR) {
          *retVal = -1;
        }
      }
      break;

    case SYS_CALL_PROCESS_WAIT_DEQUEUE:
      // P1 => queue id
      // P2 => all?
      {
        ProcessManager::Instance().WaitDequeue((int) p1, p2, false);
      }
      break;

    case SYS_CALL_PROCESS_EXIT :
			// P1 => Exit Status
			{
        auto status = (int) p1;
        ProcessManager_Exit(status);
			}
			break ;

	  case SYS_CALL_PROCESS_YIELD:
      {
        //Sleep will change process status to WAIT_SLEEP which will ensure the process is preempted
        //even if CanPreempt() returns false
        ProcessManager::Instance().Sleep(0);
      }
      break;

		case SYS_CALL_PROCESS_SLEEP :
			// P1 => Exit Status
			{
        try {
          ProcessManager::Instance().Sleep((uint64_t) p1);
          Process& process = ProcessManager::Instance().GetCurrentPAS();
          if (process.stateInfo().getError() == ProcessStateInfo::INTERRUPTED) {
            *retVal = TscClock::instance().duration(TscClock::instance().rdtsc(), process.stateInfo().sleepTsc());
            process.stateInfo().sleepTsc(0);
            process.stateInfo().setError(ProcessStateInfo::NO_ERROR);
          } else {
            *retVal = 0;
          }
        } catch(const upan::exception& e) {
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_PROCESS_GET_PS_LIST:
			//P1 - Proc List Ptr
			//P2 - List Size Ptr
			{
				PS** pProcList = ( PS**) p1;
				unsigned* uiListSize = ( unsigned*) p2;

				*retVal = 0;
        *pProcList = ProcessManager::Instance().GetProcList(*uiListSize);
			}
			break ;

		case SYS_CALL_PROCESS_FREE_PS_LIST:
			//P1 - Proc List Ptr
			//P2 - List size
			{
				PS* pProcList = ( PS*) p1;
				ProcessManager::Instance().FreeProcListMem(pProcList, p2) ;
			}
			break ;

		case SYS_CALL_PROCESS_CHILD_ALIVE :
			// P1 => PID	
			{
				*retVal = ProcessManager::Instance().IsChildAlive((int)p1) ;
			}
			break ;

    case SYS_CALL_PROCESS_ALIVE :
      // P1 => PID
      {
        *retVal = ProcessManager::Instance().IsAlive((int)p1) ;
      }
      break ;

    case SYS_CALL_PROCESS_MASK_SIGNAL:
    {
      try {
        *retVal = 0;
        auto how = (SIG_MASKING_TYPE)p1;
        auto set = (const sigset_t*)p2;
        auto oldset = (sigset_t*)p3;
        ProcessManager::Instance().MaskSignal(how, set, oldset);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_PROCESS_SIGNAL:
    {
      try {
        *retVal = 0;
        auto pid = (pid_t)p1;
        auto signo = (SIGNAL)p2;
        auto value = (const union sigval*)p3;
        ProcessManager::Instance().SendSignal(pid, signo, value);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_PROCESS_SET_SIGNAL_RETURN:
    {
      try {
        ProcessManager::Instance().SignalReturn(*(SignalTaskContext*)p1);
      } catch(const upan::exception& e) {
        KLog::exception(e);
      }
    }
    break;

    case SYS_CALL_PROCESS_SET_SIGNAL_ACTION:
    {
      try {
        *retVal = 0;
        auto signo = (SIGNAL)p1;
        auto newact = (const struct sigaction*)p2;
        auto oldact = (struct sigaction*)p3;
        ProcessManager::Instance().SetSignalAction(signo, newact, oldact);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_PROCESS_SET_SID:
    {
      try {
        auto& process = ProcessManager::Instance().GetCurrentPAS();
        *retVal = process.processID();
        process.setSID();
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_PROCESS_SET_ALARM:
    {
      *retVal = ProcessManager::Instance().SetAlarm((uint32_t)p1);
    }
    break;
  }
}

