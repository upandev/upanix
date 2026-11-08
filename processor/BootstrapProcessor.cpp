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

#include <BootstrapProcessor.h>
#include <Cpu.h>
#include <PIT.h>

extern "C" {
  extern uint32_t AP_EFER_BITS;
}

BootstrapProcessor::BootstrapProcessor(int seqId, uint32_t id, Apic& apic) : Processor(seqId, id, apic) {
  if (!enableSSE()) {
    printf("\n SSE/SSE2 is not supported!!");
    for (;;) {
      __asm__ __volatile__("hlt");
    }
  }

  AP_EFER_BITS = (uint32_t)(Cpu::MSRread(IA32_EFER) & (1ULL << 11));
  _tss = (TaskState64*)TSS_BASE_ADDR;

  //load LTR to ensure stack is setup in case if there is an exception after LIDT
  LTR();
  LIDT();
}

void BootstrapProcessor::handleTimerInterrupt(TaskContext& state) {
  incTimerTick();
  PIT::Instance().ContextSwitch(state);
}