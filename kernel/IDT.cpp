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
#include <IDT.h>
#include <MemUtil.h>
#include <MemConstants.h>
#include <MemManager.h>
#include <ProcessManager.h>
#include <PIT.h>
#include <UpanixMain.h>
#include <AsmUtil.h>

// Do not use this. else make sure to restore DS and ES
// to their initial value. Might be used for adhoc tests
#define INIT_DISPLAY \
	MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
	MemUtil_SetES(SYS_DATA_SELECTOR_DEFINED) ;

#define GEN_HANDLER(handler_msg)
//	MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
//	MemUtil_SetES(SYS_DATA_SELECTOR_DEFINED) ; \
//\
//	KC::MConsole().Message(handler_msg, ' ') ; \
//\
///*	MemUtil_MoveByte(SYS_LINEAR_SELECTOR_DEFINED, 0xb8000, '0') ; */\
///*	MemUtil_MoveByte(SYS_LINEAR_SELECTOR_DEFINED, 0xb8001, 14) ; */\
//\
//	PIT_SetContextSwitch(false) ; \
///*	__asm__ __volatile__("HLT") ;*/\
//	ProcessManager_EXIT() ; \
///*	dummy */\
//	__asm__ __volatile__("HLT") ;\

namespace {
	/************ Default Handlers *****************/
	// Handler for Interrupt 0x27 
	void DefaultHandlerForException0x27()
	{
		__asm__ __volatile__("LEAVE") ;
		__asm__ __volatile__("IRET") ;
	}

	void DefaultHandler()
	{
		MemUtil_MoveByte(SYS_LINEAR_SELECTOR_DEFINED, 0xb8000, 'U') ;
		MemUtil_MoveByte(SYS_LINEAR_SELECTOR_DEFINED, 0xb8001, 14) ;
		__asm__ __volatile__("HLT") ;
	}

	void DefaultHandler0()
	{
		GEN_HANDLER("\nDivide By Zero\n") ;
	}

	void DefaultHandler1()
	{
		GEN_HANDLER("\nException 1\n") ;
	}

	void DefaultHandler2()
	{
		GEN_HANDLER("\nException 2\n") ;
	}

	void DefaultHandler3()
	{
		GEN_HANDLER("\nException 3\n") ;
	}

	void DefaultHandler4()
	{
		GEN_HANDLER("\nException 4\n") ;
	}

	void DefaultHandler5()
	{
		GEN_HANDLER("\nException 5\n") ;
	}

	void DefaultHandler6()
	{
		GEN_HANDLER("\nException 6\n") ;
	}


	void DefaultHandler8()
	{
		GEN_HANDLER("\nException 8\n") ;
	}

	void DefaultHandler9()
	{
		GEN_HANDLER("\nException 9\n") ;
	}

	void DefaultHandlerA()
	{
		while(1)
		{
			MemUtil_MoveByte(SYS_LINEAR_SELECTOR_DEFINED, 0xb8000, 'A') ;
			MemUtil_MoveByte(SYS_LINEAR_SELECTOR_DEFINED, 0xb8001, 14) ;
			__asm__ __volatile__("IRET") ;
		}
		__asm__ __volatile__("HLT") ;
	}

	void DefaultHandlerB()
	{
		GEN_HANDLER("\nException B\n") ;
	}

	void DefaultHandlerC()
	{
		GEN_HANDLER("\nException C\n") ;
	}

	void DefaultHandlerD()
	{
		GEN_HANDLER("\nException D\n") ;
	}

	void DefaultHandlerE()
	{
		GEN_HANDLER("\nException E\n") ;
	}

	void DefaultHandlerF()
	{
		GEN_HANDLER("\nException F\n") ;
	}

	void DefaultHandler10()
	{
		GEN_HANDLER("\nException 10\n") ;
	}

	void CoProcExceptionHandler()
	{
		AsmUtil_STORE_GPR() ;

		__volatile__ unsigned short usDS = MemUtil_GetDS() ; 
		__volatile__ unsigned short usES = MemUtil_GetES() ; 
		__volatile__ unsigned short usFS = MemUtil_GetFS() ; 
		__volatile__ unsigned short usGS = MemUtil_GetGS() ;

		__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
		__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
		__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
//		__asm__ __volatile__("popw %ds") ;
//		__asm__ __volatile__("popw %fs") ;
//		__asm__ __volatile__("popw %gs") ;

		__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
//		__asm__ __volatile__("popw %es") ;

		__asm__ __volatile__("CLTS") ;

	//	__asm__ __volatile__("FNINIT") ;
		if(UpanixMain_isCoProcPresent())
		{
			__asm__ __volatile__("FNINIT") ;
		}

		__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
		__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
		__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
		__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;

		AsmUtil_RESTORE_GPR() ;

		__asm__ __volatile__("leave") ;
		__asm__ __volatile__("iret") ;
		
	}
	/************ Default Handlers *****************/
}

IDT::IDT()
{
	const int MAX_IDT_ENTRIES = 50;
	for(unsigned i = 0; i < MAX_IDT_ENTRIES; i++)
		LoadEntry(i, (uintptr_t)&DefaultHandler, SYS_CODE_SELECTOR, 0x8E) ;

	LoadDefaultHadlers() ;
	LoadInterruptTasks() ;

	IDT::IDTRegister IDTR ;

	IDTR.limit = MAX_IDT_ENTRIES * sizeof(IDT::IDTEntry) ;
	IDTR.base = IDT_BASE_ADDR ;

	__asm__ __volatile__("LIDT (%0)" : : "r"(&IDTR)) ;
  KC::MConsole().LoadMessage("IDT Initialization", Success) ;
}

void IDT::LoadInterruptTasks()
{
//	ProcessManager_BuildIntTaskState((unsigned)&MemManager_PageFaultHandlerTask, INT_TSS_LINEAR_ADDRESS_PF, MEM_PAGE_FAULT_HANDLER_STACK) ;
//	LoadEntry(0xE, 0x0, INT_TSS_SELECTOR_PF, 0xE5) ;

//	ProcessManager_BuildIntTaskState(INT_GATE_SELECTOR, (unsigned)&MemManager_PageFaultHandlerTask, SYS_CODE_SELECTOR, 0) ;
//	LoadEntry(0xE, 0x0, INT_GATE_SELECTOR, 0xEE) ;

	LoadEntry(0x7, (uintptr_t)&CoProcExceptionHandler, SYS_CODE_SELECTOR, 0x8E) ;

	LoadEntry(0xE, (uintptr_t)&MemManager::PageFaultHandlerTaskGate, SYS_CODE_SELECTOR, 0xEE) ;

	//ProcessManager_BuildIntTaskState((unsigned)&KernelService_Request, INT_TSS_LINEAR_ADDRESS_SV, MEM_KERNEL_SERVICE_STACK) ;
	//LoadEntry(0x11, 0x0, INT_TSS_SELECTOR_SV, 0xE5) ;
}

void IDT::LoadDefaultHadlers()
{
	LoadEntry(0x0, (uintptr_t)&DefaultHandler0, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x1, (uintptr_t)&DefaultHandler1, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x2, (uintptr_t)&DefaultHandler2, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x3, (uintptr_t)&DefaultHandler3, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x4, (uintptr_t)&DefaultHandler4, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x5, (uintptr_t)&DefaultHandler5, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x6, (uintptr_t)&DefaultHandler6, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x8, (uintptr_t)&DefaultHandler8, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x9, (uintptr_t)&DefaultHandler9, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0xA, (uintptr_t)&DefaultHandlerA, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0xB, (uintptr_t)&DefaultHandlerB, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0xC, (uintptr_t)&DefaultHandlerC, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0xD, (uintptr_t)&DefaultHandlerD, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0xE, (uintptr_t)&DefaultHandlerE, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0xF, (uintptr_t)&DefaultHandlerF, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(0x10, (uintptr_t)&DefaultHandler10, SYS_CODE_SELECTOR, 0x8E) ;

	//Spurious IRQ
	LoadEntry(0x27, (uintptr_t)&DefaultHandlerForException0x27, SYS_CODE_SELECTOR, 0x8E) ;
}

void IDT::LoadEntry(unsigned uiIDTNo, uintptr_t uiOffset, unsigned short usSelector, byte bOptions)
{
	IDT::IDTEntry* idtEntry = reinterpret_cast<IDT::IDTEntry*>(IDT_BASE_ADDR) + uiIDTNo ;
	
//	__asm__ __volatile__("push %ds") ;
//	MemUtil_SetDS(SYS_LINEAR_SELECTOR_DEFINED) ;

	idtEntry->lowerOffset = uiOffset & 0x0000FFFF ;
	idtEntry->selector = usSelector ;
	idtEntry->unused = 0 ;
	idtEntry->options = bOptions ;
	idtEntry->upperOffset = (uiOffset & 0xFFFF0000) >> 16 ;

//	__asm__ __volatile__("pop %ds") ;
}

