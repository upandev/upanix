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
#include <Cpu.h>

uint64_t SYSCALL_USER_ORIG_RSP = PROCESS_STACK_TOP_ADDRESS - 8 * 1;
uint64_t SYSCALL_USER_LOCAL_RSP = PROCESS_STACK_TOP_ADDRESS - 8 * 2;
uint64_t SYSCALL_RETURN_ADDRESS = PROCESS_STACK_TOP_ADDRESS - 8 * 3;
uint64_t SYSCALL_STACK_TOP = SYSCALL_RETURN_ADDRESS;

typedef void Handler(uint64_t* retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5);
typedef bool Check(uint64_t uiSysCallID);

typedef struct
{
	Check* pFuncCheck ;	
	Handler* pFuncHandle ;
} SysCallHandler ;

/************************ static **********************************/
static SysCallHandler SysCall_Handlers[10] ; 
static int SysCall_NoOfHandlers ;

void SysCall_InitializeHandler(SysCallHandler* pSysCallHandler, Check* pFuncCheck, Handler* pFuncHandle)
{
	pSysCallHandler->pFuncCheck = pFuncCheck ;
	pSysCallHandler->pFuncHandle = pFuncHandle ;
}

/******************************************************************/

constexpr uint32_t IA32_EFER = 0xC0000080;
constexpr uint32_t IA32_STAR = 0xC0000081;
constexpr uint32_t IA32_LSTAR = 0xC0000082;
constexpr uint32_t IA32_FMASK = 0xC0000084;

extern "C" {
  void _syscall_handler();
}

void SysCall_Initialize() {
  if (!Cpu::Instance().HasSupport(CF_MSR) || !Cpu::Instance().HasSupport(CF_SYSENTEREXIT)) {
    throw upan::exception(XLOC, "MSR support is required to support system calls");
  }

  //enable syscall/sysret instructions in 64bit mode
  Cpu::Instance().MSRwrite(IA32_EFER, Cpu::Instance().MSRread(IA32_EFER) | 1);
  Cpu::Instance().MSRwrite(IA32_FMASK, 0x200);
  Cpu::Instance().MSRwrite(IA32_LSTAR, (uintptr_t)&_syscall_handler);
  //As per intel documentation,
  //For Ring0->Ring3 (entry), the SS selector is obtained by adding 8 to the CS selector value in STAR [47:32]
  //For Ring3->Ring0 (return), the CS selector is obtained by adding 16 to STAR [63:48] and SS selector by adding 8 to STAR [63:48]
  Cpu::Instance().MSRwrite(IA32_STAR, ((uint64_t)(SYS_DATA_SELECTOR | 0x3) << 48) | ((uint64_t)SYS_CODE_SELECTOR << 32));

	SysCall_NoOfHandlers = 0 ;

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallDisplay_IsPresent, &SysCallDisplay_Handle);

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallFile_IsPresent, &SysCallFile_Handle);

  SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallIO_IsPresent, &SysCallFile_Handle);

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallProc_IsPresent, &SysCallProc_Handle);

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallMem_IsPresent, &SysCallMem_Handle);

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallDrive_IsPresent, &SysCallDrive_Handle);

	SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallUtil_IsPresent, &SysCallUtil_Handle);

  SysCall_InitializeHandler(&SysCall_Handlers[SysCall_NoOfHandlers++], &SysCallNet_IsPresent, &SysCallNet_Handle);

  KC::MConsole().LoadMessage("SysCall Initialization", Success) ;
}

uint64_t SYS_CALL_ID = 0;

upan::map<uint64_t, upan::map<int, int>>& get_syscall_stats() {
  static upan::map<uint64_t, upan::map<int, int>> syscall_stats;
  return syscall_stats;
}

extern "C" void SysCall_Entry(uint64_t sysCallId, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5) {
  get_syscall_stats()[sysCallId][getpid()]++;
  //printf("\n System Call Params: %lu, %llx, %llx, %llx, %llx, %llx\n", sysCallId, p1, p2, p3, p4, p5);
  SYS_CALL_ID = sysCallId;
	uint64_t retVal = 0;
	for(auto i = 0; i < SysCall_NoOfHandlers; ++i) {
		if(SysCall_Handlers[i].pFuncCheck(sysCallId)) {
      try {
  			SysCall_Handlers[i].pFuncHandle(&retVal, sysCallId, true, p1, p2, p3, p4, p5) ;
      } catch(const upan::exception& ex) {
        printf("\n SysCall %lu failed with error: %s\n", sysCallId, ex.ErrorMsg().c_str());
        retVal = -1;
      }
	  	break ;
		}
	}
  *((uint64_t*)SYSCALL_RETURN_ADDRESS) = retVal;
}

