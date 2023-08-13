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

extern "C" {
TaskContext *timer_interrupt_handler(TaskContext *state) {
  PIT::Instance().Handler();
  return state;
}
}

__attribute__((interrupt)) void page_fault_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  MemManager::PageFaultHandlerTaskGate(errorCode);
}

__attribute__((interrupt)) void keyboard_interrupt_handler(InterruptState* state) {
  PS2KeyboardDriver::Handler();
}

__attribute__((interrupt)) void mouse_interrupt_handler(InterruptState* state) {
  PS2MouseDriver::Handler();
}

__attribute__((interrupt)) void rtc_interrupt_handler(InterruptState* state) {
  RTC::Handler();
}

__attribute__((interrupt)) void xhci_interrupt_handler(InterruptState* state) {
  XHCIManager::Handler();
}

/************ Default Handlers *****************/
// Handler for Interrupt 0x27
__attribute__((interrupt)) void isr_0x27_interrupt_handler(InterruptState* state) {
}

__attribute__((interrupt)) void isr_default_interrupt_handler(InterruptState* state) {
  printf("\n default exception handler invoked!\n");
  __asm__ __volatile__("HLT") ;
}

__attribute__((interrupt)) void isr_0_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 0: Divide By Zero\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_1_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 1: Debug Exception\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_2_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 2: NMI\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_3_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 3: Break Point\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_4_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 4: Overflow Exception\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_5_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 5: Range out of bounds exception\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_6_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 6: Invalid Opcode Exception\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_7_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 7: Device not available Exception\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_8_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 8: Double Fault Exception. Error Code: 0x%x\n", errorCode);
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_9_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 9: Coprocessor Segment Overrun\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_10_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 10: Invalid TSS Exception. Error Code: 0x%x\n", errorCode);
  __asm__ __volatile__("HLT");
}

__attribute__((interrupt)) void isr_11_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 11: Segment not present. Error Code: 0x%x\n", errorCode);
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_12_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 12: Stack Fault Exception. Error Code: 0x%x\n", errorCode);
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_13_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 13: General Protection Fault. Error Code: 0x%x\n", errorCode);
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_16_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 16: x87 FPU Floating-Point Error\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_17_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 17: Alignment Check Exception. Error Code: 0x%x\n", errorCode);
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_18_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 18: Machine Check Exception.\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_19_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 19: SIMD Floating-Point Exception.\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_20_interrupt_handler(InterruptState* state) {
  printf("\n Interrupt 20: Virtualization Exception.\n");
  ProcessManager_Exit();
}

__attribute__((interrupt)) void isr_21_interrupt_handler(InterruptState* state, uint64_t errorCode) {
  printf("\n Interrupt 21: Control Protection Exception. Error Code: 0x%x\n", errorCode);
  ProcessManager_Exit();
}
