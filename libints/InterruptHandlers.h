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

#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} PACKED InterruptState;

typedef struct {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rbp;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;
  InterruptState interruptState;
} PACKED TaskContext;

extern "C" {
  TaskContext *timer_interrupt_handler(TaskContext *state);
}

void page_fault_interrupt_handler(InterruptState* state, uint64_t errorCode);
void keyboard_interrupt_handler(InterruptState* state);
void mouse_interrupt_handler(InterruptState* state);
void rtc_interrupt_handler(InterruptState* state);
void xhci_interrupt_handler(InterruptState* state);

void isr_0x27_interrupt_handler(InterruptState* state);
void isr_default_interrupt_handler(InterruptState* state);
void isr_0_interrupt_handler(InterruptState* state);
void isr_1_interrupt_handler(InterruptState* state);
void isr_2_interrupt_handler(InterruptState* state);
void isr_3_interrupt_handler(InterruptState* state);
void isr_4_interrupt_handler(InterruptState* state);
void isr_5_interrupt_handler(InterruptState* state);
void isr_6_interrupt_handler(InterruptState* state);
void isr_7_interrupt_handler(InterruptState* state);
void isr_8_interrupt_handler(InterruptState* state, uint64_t errorCode);
void isr_9_interrupt_handler(InterruptState* state);
void isr_10_interrupt_handler(InterruptState* state, uint64_t errorCode);
void isr_11_interrupt_handler(InterruptState* state, uint64_t errorCode);
void isr_12_interrupt_handler(InterruptState* state, uint64_t errorCode);
void isr_13_interrupt_handler(InterruptState* state, uint64_t errorCode);
void isr_16_interrupt_handler(InterruptState* state);
void isr_17_interrupt_handler(InterruptState* state, uint64_t errorCode);
void isr_18_interrupt_handler(InterruptState* state);
void isr_19_interrupt_handler(InterruptState* state);
void isr_20_interrupt_handler(InterruptState* state);
void isr_21_interrupt_handler(InterruptState* state, uint64_t errorCode);