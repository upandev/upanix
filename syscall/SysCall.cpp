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
#include <SysCall.h>
#include <MemUtil.h>
#include <exception.h>

typedef void Handler(
__volatile__ int* piRetVal,
__volatile__ unsigned uiSysCallID, 
__volatile__ bool bDoAddrTranslation,
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9) ;

typedef byte Check(unsigned uiSysCallID) ;

typedef struct
{
	Check* pFuncCheck ;	
	Handler* pFuncHandle ;
} SysCallHandler ;

/************************ static **********************************/
static SysCallHandler SysCall_Handlers[10] ; 
static unsigned SysCall_NoOfHandlers ;

void SysCall_InitializeHandler(SysCallHandler* pSysCallHandler, Check* pFuncCheck, Handler* pFuncHandle)
{
	pSysCallHandler->pFuncCheck = pFuncCheck ;
	pSysCallHandler->pFuncHandle = pFuncHandle ;
}

/******************************************************************/

void SysCall_Initialize()
{
	ProcessManager::Instance().BuildCallGate(CALL_GATE_SELECTOR, (unsigned)&SysCall_Entry, SYS_CODE_SELECTOR, NO_OF_SYSCALL_PARAMS) ;

	SysCall_NoOfHandlers = 0 ;

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallDisplay_IsPresent, &SysCallDisplay_Handle) ;

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallFile_IsPresent, &SysCallFile_Handle) ;

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallProc_IsPresent, &SysCallProc_Handle) ;
	
	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallMem_IsPresent, &SysCallMem_Handle) ;

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallDrive_IsPresent, &SysCallDrive_Handle) ;

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallUtil_IsPresent, &SysCallUtil_Handle) ;

  KC::MConsole().LoadMessage("SysCall Initialization", Success) ;
}

__volatile__ int SYS_CALL_ID ;

void SysCall_Entry(__volatile__ unsigned uiCSCorrection,
__volatile__ unsigned uiSysCallID, 
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9)
{
	AsmUtil_STORE_GPR() ;
	
	__volatile__ unsigned short usDS = MemUtil_GetDS() ; 
	__volatile__ unsigned short usES = MemUtil_GetES() ; 
	__volatile__ unsigned short usFS = MemUtil_GetFS() ; 
	__volatile__ unsigned short usGS = MemUtil_GetGS() ;

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
//	__asm__ __volatile__("popw %ds") ;
//	__asm__ __volatile__("popw %fs") ;
//	__asm__ __volatile__("popw %gs") ;

//	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ;
//	__asm__ __volatile__("popw %es") ;

	__volatile__ int iRetVal = 0;

	//printf(", SC: %u", uiSysCallID) ;
	SYS_CALL_ID = uiSysCallID ;
	
	for(unsigned i = 0; i < SysCall_NoOfHandlers; i++)
	{
		if(SysCall_Handlers[i].pFuncCheck(uiSysCallID))
		{
      try
      {
  			SysCall_Handlers[i].pFuncHandle(&iRetVal, uiSysCallID, true, uiP1, uiP2, uiP3, uiP4, uiP5, uiP6, uiP7, uiP8, uiP9) ;
      }
      catch(const upan::exception& ex)
      {
        printf("\n SysCall %u failed with error: %s\n", uiSysCallID, ex.ErrorMsg().c_str());
        iRetVal = -1;
      }
	  	break ;
		}
	}

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;

	AsmUtil_RESTORE_GPR() ;

	__asm__ __volatile__("movl %0, %%eax" : : "m"(iRetVal)) ;
	__asm__ __volatile__("leave") ;
	__asm__ __volatile__("lret %0" : : "i"(NO_OF_SYSCALL_PARAMS * 4)) ; 

	/* optional arg to ret = number of bytes consumed by
		args pushed on to stack by calling program 
		This is to adjust old SS:ESP ---> the one of calling program */
}

