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
# include <exception.h>

byte SysCallProc_IsPresent(uint64_t sysCallId)
{
	return (sysCallId > SYS_CALL_PROC_START && sysCallId < SYS_CALL_PROC_END) ;
}

void
SysCallProc_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3,
                   uint64_t p4, uint64_t p5)
{
	switch(sysCallId)
	{
		case SYS_CALL_DLL_RELOCATE :
			// P8 => Second Entry in GOT
			// P9 => Relocation Table Offset
      try
      {
        //ProcessManager_DisableTaskSwitch() ;
        DynamicLinkLoader_DoRelocation(&ProcessManager::Instance().GetCurrentPAS(), (int)p4, p5, retVal);
        //ProcessManager_EnableTaskSwitch() ;
      }
      catch(const upan::exception& e)
      {
        printf("\n Dynamic Relocation Failed: %s", e.ErrorMsg().c_str());
      }
			break ;

		case SYS_CALL_PROCESS_EXEC :
			// P1 => Address of File Name Char Array
			// P2 => Number Of Args
			// P3 => Arg Vector Address
			{
				//ProcessManager_DisableTaskSwitch() ;
				
				char* szFile = KERNEL_ADDR(doAddrTranslation, char*, p1) ;
				char** szArgList = KERNEL_ADDR(doAddrTranslation, char**, p3) ;

				for(unsigned i = 0; i < p2; i++)
					szArgList[i] = KERNEL_ADDR(doAddrTranslation, char*, szArgList[i]) ;

				*retVal = KC::MKernelService().RequestProcessExec(szFile, p2, (const char**)szArgList) ;

				//ProcessManager_EnableTaskSwitch() ;
			}
			break ;

	  case SYS_CALL_THREAD_EXEC :
	    // P1 => thread entry point
	    // P2 => thread (input) arg
      {
        *retVal = KC::MKernelService().RequestThreadExec(p1, p2, (void*)p3) ;
      }
      break;
		case SYS_CALL_PROCESS_WAIT_PID :
			// P1 => PID	
			{
				ProcessManager::Instance().WaitOnChild((int)p1) ;
			}
			break ;

		case SYS_CALL_PROCESS_EXIT :
			// P1 => Exit Status
			{
				ProcessManager_Exit() ;
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
				ProcessManager::Instance().Sleep((unsigned)p1) ;
			}
			break ;

		case SYS_CALL_PROCESS_PID :
			{
				*retVal = ProcessManager::GetCurrentProcessID();
			}
			break ;

		case SYS_CALL_PROCESS_GET_ENV:
			//P1 - Env Var
			{
				char* szVar = KERNEL_ADDR(doAddrTranslation, char*, p1) ;
				const auto& val = ProcessManager::Instance().GetCurrentPAS().getEnv(szVar);
				if (val.isEmpty()) {
				  *retVal = -1;
				} else {
          char* szVal = KERNEL_ADDR(doAddrTranslation, char*, p2) ;
          strcpy(szVal, val.value().c_str());
          *retVal = 0;
				}
			}
			break ;

		case SYS_CALL_PROCESS_SET_ENV:
			//P1 - Env Var
			//P2 - Env Val
			{
				char* szVar = KERNEL_ADDR(doAddrTranslation, char*, p1) ;
				char* szVal = KERNEL_ADDR(doAddrTranslation, char*, p2) ;

				*retVal = 0 ;
				try {
				  ProcessManager::Instance().GetCurrentPAS().setEnv(szVar, szVal);
				} catch(const upan::exception& e) {
				  e.Print();
				  *retVal = -1;
				}
			}
			break ;

		case SYS_CALL_PROCESS_GET_PS_LIST:
			//P1 - Proc List Ptr
			//P2 - List Size Ptr
			{
				PS** pProcList = KERNEL_ADDR(doAddrTranslation, PS**, p1) ;
				unsigned* uiListSize = KERNEL_ADDR(doAddrTranslation, unsigned*, p2) ;

				*retVal = 0;
        *pProcList = ProcessManager::Instance().GetProcList(*uiListSize);
			}
			break ;

		case SYS_CALL_PROCESS_FREE_PS_LIST:
			//P1 - Proc List Ptr
			//P2 - List size
			{
				PS* pProcList = KERNEL_ADDR(doAddrTranslation, PS*, p1) ;
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
  }
}

