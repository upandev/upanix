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

/* Intel 8253 Programmable Interval Timer */

#include <PIT.h>
#include <PIC.h>
#include <IDT.h>
#include <AsmUtil.h>
#include <PortCom.h>
#include <MemConstants.h>
#include <MemUtil.h>
#include <mutex.h>
#include <Apic.h>
#include <atomicop.h>

static __volatile__ unsigned PIT_ClockCountForSleep ;
static __volatile__ unsigned char Process_bContextSwitch ;
static __volatile__ uint32_t Process_iTaskSwitch ;

void PIT_Initialize()
{
  ReturnCode status = Success;
	PIT_ClockCountForSleep = 0 ;
	Process_bContextSwitch = false ;
	Process_iTaskSwitch = 1;

  IrqGuard g;
  if(!IrqManager::Instance().IsApic())
  {
  	unsigned uiTimerRate = TIMECOUNTER_i8254_FREQU / INT_PER_SEC ;

	  PortCom_SendByte(PIT_MODE_PORT, 0x34) ;				// Set Timer to Mode 2 -- Free Running LSB/MSB
  	PortCom_SendByte(PIT_COUNTER_0_PORT, uiTimerRate & 0xFF) ;			// Clock Divisor LSB
	  PortCom_SendByte(PIT_COUNTER_0_PORT, (uiTimerRate >> 8) & 0xFF) ;	// Clock Divisor MSB
  }

	if(!IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().TIMER_IRQ, (uintptr_t)&PIT_Handler))
    status = Failure;

  KC::MConsole().LoadMessage("Timer Initialization", status);
}

unsigned PIT_GetClockCount() { return PIT_ClockCountForSleep ; }

unsigned char PIT_IsContextSwitch() { return Process_bContextSwitch ; }
void PIT_SetContextSwitch(bool flag) { Process_bContextSwitch = flag ; }

bool PIT_IsTaskSwitch() {
  return Process_iTaskSwitch == 1;
}

//return true if it was previously disabled and now enabled
bool PIT_EnableTaskSwitch() {
  return upan::atomic::op::swap(Process_iTaskSwitch, 1) == 0;
}

//return true if it was previously enabled and now disabled
bool PIT_DisableTaskSwitch() {
  return upan::atomic::op::swap(Process_iTaskSwitch, 0) == 1;
}

void PIT_Handler()
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

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
//	__asm__ __volatile__("popw %es") ;

	// 1 Int --> 1ms
	++PIT_ClockCountForSleep;
	if((PIT_ClockCountForSleep % 10) == 0 && PIT_IsTaskSwitch())
	{
		__volatile__ unsigned uiTaskReg = 0;
		__asm__ __volatile__("STR %ax") ;
		__asm__ __volatile__("movw %%ax, %0" : "=m"(uiTaskReg) :) ;

		if(uiTaskReg == USER_TSS_SELECTOR) 
		{
			Process_bContextSwitch = true ;

//			__asm__ __volatile__("pushf") ;
//			__asm__ __volatile__("popl %eax") ;
			__asm__ __volatile__("mov $0x4000, %ebx") ;
			__asm__ __volatile__("or %ebx, %eax") ;
//			__asm__ __volatile__("pushl %eax") ;
//			__asm__ __volatile__("popf") ;
			
			IrqManager::Instance().SendEOI(StdIRQ::Instance().TIMER_IRQ);

			__asm__ __volatile__("IRET") ;
		
//			__asm__ __volatile__("pushf") ;
//			__asm__ __volatile__("popl %eax") ;
			__asm__ __volatile__("mov $0xBFFF, %ebx") ;
			__asm__ __volatile__("and %ebx, %eax") ;
//			__asm__ __volatile__("pushl %eax") ;
//			__asm__ __volatile__("popf") ;

			//AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
			__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
			__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
			__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
			__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;
	
			AsmUtil_RESTORE_GPR() ;

			__asm__ __volatile__("leave") ;
			__asm__ __volatile__("IRET") ;
		}
	}

  IrqManager::Instance().SendEOI(StdIRQ::Instance().TIMER_IRQ);

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;

	AsmUtil_RESTORE_GPR() ;

	__asm__ __volatile__("leave") ;
	__asm__ __volatile__("IRET") ;
}

unsigned PIT_RoundSleepTime(__volatile__ unsigned uiSleepTime)
{
  return uiSleepTime;
//	if((uiSleepTime % 10) >= 5)
//		return uiSleepTime / 10 + 1 ;
//
//	return uiSleepTime / 10 ;
}

