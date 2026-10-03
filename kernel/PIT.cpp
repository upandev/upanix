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
#include <ProcessManager.h>
#include <Acpi.h>

extern "C" {
  void _pit_timer_interrupt_handler();
}

PIT::PIT() : _pitIrq(&StdIRQ::Instance().TIMER_IRQ) {
}

void PIT::Initialize() {
  ReturnCode status = Success;

  IrqGuard g;

  uint32_t uiTimerRate = TIMECOUNTER_i8254_FREQU / INT_PER_SEC ;
  PortCom_SendByte(PIT_MODE_PORT, 0x34) ;				// Set Timer to Mode 2 -- Free Running LSB/MSB
  PortCom_SendByte(PIT_COUNTER_0_PORT, uiTimerRate & 0xFF) ;			// Clock Divisor LSB
  PortCom_SendByte(PIT_COUNTER_0_PORT, (uiTimerRate >> 8) & 0xFF) ;	// Clock Divisor MSB

  if (IrqManager::Instance().IsApic()) {
    const auto& apicMapping = Acpi::Instance().GetMadt().GetIntSourceOverride(StdIRQ::Instance().TIMER_IRQ.GetIRQNo());
    if (apicMapping.isEmpty()) {
      printf("\n No APIC mapping found for PIT timer interrupt!");
      status = Failure;
    } else {
      _pitIrq = IrqManager::Instance().RegisterIRQ(apicMapping.value(), (uintptr_t)&_pit_timer_interrupt_handler);
      if (_pitIrq) {
        IrqManager::Instance().EnableIRQ(*_pitIrq);
      } else {
        printf("\n Failed to register PIT Timer IRQ on mapped APIC IRQ %d", apicMapping.value());
        status = Failure;
      }
    }
  } else {
    if (IrqManager::Instance().RegisterIRQ(*_pitIrq, (uintptr_t)&_pit_timer_interrupt_handler)) {
      IrqManager::Instance().EnableIRQ(*_pitIrq);
    } else {
      printf("\n Failed to register PIT Timer IRQ %d", _pitIrq->GetIRQNo());
      status = Failure;
    }
  }

  KC::MConsole().LoadMessage("Timer Initialization", status);
}

void PIT::ContextSwitchHandler(TaskContext& taskContext) {
  SetKernelMode(true);
  ProcessManager::Instance().ContextSwitch(taskContext);
  SetKernelMode(false);
  IrqManager::Instance().SendEOI(StdIRQ::Instance().TIMER_IRQ);
}

void PIT::disable() {
  IrqManager::Instance().DisableIRQ(*_pitIrq);
  printf("\n PIT Timer Disabled");
}

void PIT::Handler() {
  // 1 Int --> 1ms
  IrqManager::Instance().SendEOI(*_pitIrq);
}
