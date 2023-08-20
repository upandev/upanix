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

byte SysCallProc_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_PROC_START && uiSysCallID < SYS_CALL_PROC_END) ;
}

void SysCallProc_Handle(
__volatile__ int* piRetVal,
__volatile__ uintptr_t uiSysCallID,
__volatile__ bool bDoAddrTranslation,
__volatile__ uintptr_t uiP1,
__volatile__ uintptr_t uiP2,
__volatile__ uintptr_t uiP3,
__volatile__ uintptr_t uiP4,
__volatile__ uintptr_t uiP5,
__volatile__ uintptr_t uiP6,
__volatile__ uintptr_t uiP7,
__volatile__ uintptr_t uiP8,
__volatile__ uintptr_t uiP9)
{
	switch(uiSysCallID)
	{
		case SYS_CALL_DLL_RELOCATE :
			// P8 => Second Entry in GOT
			// P9 => Relocation Table Offset
      try
      {
        //ProcessManager_DisableTaskSwitch() ;
        DynamicLinkLoader_DoRelocation(&ProcessManager::Instance().GetCurrentPAS(), (int)uiP8, uiP9, piRetVal);
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
				
				char* szFile = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				char** szArgList = KERNEL_ADDR(bDoAddrTranslation, char**, uiP3) ;

				for(unsigned i = 0; i < uiP2; i++)
					szArgList[i] = KERNEL_ADDR(bDoAddrTranslation, char*, szArgList[i]) ;

				*piRetVal = KC::MKernelService().RequestProcessExec(szFile, uiP2, (const char**)szArgList) ;

				//ProcessManager_EnableTaskSwitch() ;
			}
			break ;

	  case SYS_CALL_THREAD_EXEC :
	    // P1 => thread entry point
	    // P2 => thread (input) arg
      {
        *piRetVal = KC::MKernelService().RequestThreadExec(uiP1, uiP2, (void*)uiP3) ;
      }
      break;
		case SYS_CALL_PROCESS_WAIT_PID :
			// P1 => PID	
			{
				ProcessManager::Instance().WaitOnChild((int)uiP1) ;
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
        ProcessManager::Instance().Sleep(1) ;
        //ProcessManager_Yield();
      }
      break;

		case SYS_CALL_PROCESS_SLEEP :
			// P1 => Exit Status
			{
				ProcessManager::Instance().Sleep((unsigned)uiP1) ;
			}
			break ;

		case SYS_CALL_PROCESS_PID :
			{
				*piRetVal = ProcessManager::GetCurrentProcessID();
			}
			break ;

		case SYS_CALL_PROCESS_GET_ENV:
			//P1 - Env Var
			{
				char* szVar = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				const auto& val = ProcessManager::Instance().GetCurrentPAS().getEnv(szVar);
				if (val.isEmpty()) {
				  *piRetVal = -1;
				} else {
          char* szVal = KERNEL_ADDR(bDoAddrTranslation, char*, uiP2) ;
          strcpy(szVal, val.value().c_str());
          *piRetVal = 0;
				}
			}
			break ;

		case SYS_CALL_PROCESS_SET_ENV:
			//P1 - Env Var
			//P2 - Env Val
			{
				char* szVar = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1) ;
				char* szVal = KERNEL_ADDR(bDoAddrTranslation, char*, uiP2) ;

				*piRetVal = 0 ;
				try {
				  ProcessManager::Instance().GetCurrentPAS().setEnv(szVar, szVal);
				} catch(const upan::exception& e) {
				  e.Print();
				  *piRetVal = -1;
				}
			}
			break ;

		case SYS_CALL_PROCESS_GET_PS_LIST:
			//P1 - Proc List Ptr
			//P2 - List Size Ptr
			{
				PS** pProcList = KERNEL_ADDR(bDoAddrTranslation, PS**, uiP1) ;
				unsigned* uiListSize = KERNEL_ADDR(bDoAddrTranslation, unsigned*, uiP2) ;

				*piRetVal = 0;
        *pProcList = ProcessManager::Instance().GetProcList(*uiListSize);
			}
			break ;

		case SYS_CALL_PROCESS_FREE_PS_LIST:
			//P1 - Proc List Ptr
			//P2 - List size
			{
				PS* pProcList = KERNEL_ADDR(bDoAddrTranslation, PS*, uiP1) ;
				ProcessManager::Instance().FreeProcListMem(pProcList, uiP2) ;
			}
			break ;

		case SYS_CALL_PROCESS_CHILD_ALIVE :
			// P1 => PID	
			{
				*piRetVal = ProcessManager::Instance().IsChildAlive((int)uiP1) ;
			}
			break ;

    case SYS_CALL_PROCESS_ALIVE :
      // P1 => PID
      {
        *piRetVal = ProcessManager::Instance().IsAlive((int)uiP1) ;
      }
      break ;
  }
}

