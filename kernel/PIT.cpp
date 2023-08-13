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
#include <PortCom.h>
#include <atomicop.h>

extern "C" {
  void _timer_interrupt_handler();
}

PIT::PIT() : _clockCountForSleep(0) {
}

void PIT::Initialize() {
  ReturnCode status = Success;
  IrqGuard g;
  if(!IrqManager::Instance().IsApic()) {
  	uint32_t uiTimerRate = TIMECOUNTER_i8254_FREQU / INT_PER_SEC ;
	  PortCom_SendByte(PIT_MODE_PORT, 0x34) ;				// Set Timer to Mode 2 -- Free Running LSB/MSB
  	PortCom_SendByte(PIT_COUNTER_0_PORT, uiTimerRate & 0xFF) ;			// Clock Divisor LSB
	  PortCom_SendByte(PIT_COUNTER_0_PORT, (uiTimerRate >> 8) & 0xFF) ;	// Clock Divisor MSB
  }

	if(!IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().TIMER_IRQ, (uintptr_t)&_timer_interrupt_handler))
    status = Failure;

  KC::MConsole().LoadMessage("Timer Initialization", status);
}

void PIT::Handler() {
	// 1 Int --> 1ms
	++_clockCountForSleep;
	if ((_clockCountForSleep % 1000) == 0)
	  printf("\n TIMER INT %d", _clockCountForSleep);

//	if((PIT_ClockCountForSleep % 10) == 0 && PIT_IsTaskSwitch())
//	{
//		__volatile__ unsigned uiTaskReg = 0;
//		__asm__ __volatile__("STR %ax") ;
//		__asm__ __volatile__("movw %%ax, %0" : "=m"(uiTaskReg) :) ;
//
//		if(uiTaskReg == USER_TSS_SELECTOR)
//		{
//			Process_bContextSwitch = true ;
//
////			__asm__ __volatile__("pushf") ;
////			__asm__ __volatile__("popl %eax") ;
//			__asm__ __volatile__("mov $0x4000, %ebx") ;
//			__asm__ __volatile__("or %ebx, %eax") ;
////			__asm__ __volatile__("pushl %eax") ;
////			__asm__ __volatile__("popf") ;
//
//			IrqManager::Instance().SendEOI(StdIRQ::Instance().TIMER_IRQ);
//
//			__asm__ __volatile__("IRET") ;
//
////			__asm__ __volatile__("pushf") ;
////			__asm__ __volatile__("popl %eax") ;
//			__asm__ __volatile__("mov $0xBFFF, %ebx") ;
//			__asm__ __volatile__("and %ebx, %eax") ;
////			__asm__ __volatile__("pushl %eax") ;
////			__asm__ __volatile__("popf") ;
//
//			//AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
//			__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
//			__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
//			__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
//			__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;
//
//			AsmUtil_RESTORE_GPR() ;
//
//			__asm__ __volatile__("leave") ;
//			__asm__ __volatile__("IRETQ") ;
//		}
//	}

  IrqManager::Instance().SendEOI(StdIRQ::Instance().TIMER_IRQ);
}

uint32_t PIT::RoundSleepTime(__volatile__ uint32_t uiSleepTime)
{
  return uiSleepTime;
//	if((uiSleepTime % 10) >= 5)
//		return uiSleepTime / 10 + 1 ;
//
//	return uiSleepTime / 10 ;
}

