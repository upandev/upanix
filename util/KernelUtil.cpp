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
#include <PIT.h>
#include <IrqManager.h>
#include <ProcessManager.h>
#include <KernelUtil.h>

void KernelUtil::Wait(__volatile__ unsigned uiTimeInMilliSec)
{
	uiTimeInMilliSec = PIT::Instance().RoundSleepTime(uiTimeInMilliSec) ;
	__volatile__ unsigned uiStartTime = PIT::Instance().GetClockCount() ;

	IrqManager::Instance().EnableIRQ(StdIRQ::Instance().TIMER_IRQ) ;
	while((PIT::Instance().GetClockCount() - uiStartTime) < uiTimeInMilliSec)
	{
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::WaitOnInterrupt(const IRQ& irq)
{
	while(!irq.Consume())
	{	
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::TightLoopWait(unsigned loop)
{
	unsigned i ;
	for(i = 0; i < loop * loop * loop; i++)
	{
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::ScheduleTimedTask(const char* szName, unsigned uiTimeInMilliSec, TimerTask& task) {
  upan::vector<uintptr_t> params;
  params.push_back(uiTimeInMilliSec);
  params.push_back((uintptr_t)&task);
  ProcessManager::Instance().CreateKernelProcess(szName, (uintptr_t) &SystemTimer, ProcessManager::GetCurrentProcessID(), false, params);
}

void KernelUtil::SystemTimer(unsigned timeInMilliSec, TimerTask* task)
{
	do
	{
		ProcessManager::Instance().Sleep(timeInMilliSec) ;
  } while(task->TimerTrigger()) ;
	ProcessManager_Exit();
}

