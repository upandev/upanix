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

#include <InterruptHandlers.h>
#include <PIT.h>
#include <stdio.h>
#include <ProcessManager.h>
#include <PS2KeyboardDriver.h>
#include <PS2MouseDriver.h>
#include <RTC.h>
#include <XHCIManager.h>
#include <PortCom.h>

extern "C" {
  void timer_interrupt_handler(TaskContext *state) {
    PIT::Instance().ContextSwitchHandler(*state);
  }

  void pit_timer_interrupt_handler() {
    PIT::Instance().Handler();
  }

  void keyboard_interrupt_handler() {
    PS2KeyboardDriver::Handler();
  }

  void mouse_interrupt_handler() {
    PS2MouseDriver::Handler();
  }

  void rtc_interrupt_handler() {
    RTC::Handler();
  }

  void xhci_interrupt_handler() {
    XHCIManager::Handler();
  }

  void page_fault_interrupt_handler() {
    MemManager::PageFaultHandler();
  }

  void isr_0x27_interrupt_handler() {
    COM1::Instance().Write("\nInt 0x27.");
  }
}

/************ Default Handlers *****************/
__attribute__((interrupt)) void isr_default_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n default exception handler invoked!");
  __asm__ __volatile__("hlt") ;
}

__attribute__((interrupt)) void isr_0_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 0: Divide By Zero.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_1_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 1: Debug Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_2_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 2: NMI.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_3_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 3: Break Point.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_4_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 4: Overflow Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_5_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 5: Range out of bounds exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_6_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 6: Invalid Opcode Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_7_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 7: Device not available Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_8_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 8: Double Fault Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_9_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 9: Coprocessor Segment Overrun.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_10_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 10: Invalid TSS Exception.");
  __asm__ __volatile__("HLT");
}

__attribute__((interrupt)) void isr_11_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 11: Segment not present.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_12_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 12: Stack Fault Exception");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_13_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 13: General Protection Fault.");
//  printf("\n Interrupt State: %x, %x, %x, %x, %x\n", state->cs, state->rip, state->ss, state->rsp, state->rflags);
//  printf("\n Error Code: %lu\n", errorCode);
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_16_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 16: x87 FPU Floating-Point Error.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_17_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 17: Alignment Check Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_18_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 18: Machine Check Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_19_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 19: SIMD Floating-Point Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_20_interrupt_handler(InterruptState* state) {
  COM1::Instance().Write("\n Interrupt 20: Virtualization Exception.");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_21_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  COM1::Instance().Write("\n Interrupt 21: Control Protection Exception.");
  ProcessManager_Exit();
}