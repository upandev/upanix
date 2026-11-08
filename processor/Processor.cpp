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

#include <Processor.h>
#include <IDT.h>
#include <Cpu.h>
#include <ProcessConstants.h>

Processor::Processor(int seqId, uint32_t id, Apic& apic) : _seqId(seqId), _id(id), _apic(apic), _tss(nullptr),
  _kernelMode(true), _currentProcessId(NO_PROCESS_ID), _timerTicks(0) {
}

bool Processor::enableSSE() {
  uint64_t result = 0;
  // Test if the CPU supports the CPUID-Command
  __asm__ __volatile__("pushfq;"
                       "pushfq;"
                       "popq %%rax;"
                       "movq %%rax, %%rcx;"
                       "xorq $0x200000, %%rax;"
                       "pushq %%rax;"
                       "popfq;"
                       "pushfq;"
                       "popq %%rax;"
                       "xorq %%rcx, %%rax;"
                       "shrq $21, %%rax;"
                       "popfq;"
                       "movq %%rax, %0;" : "=r"(result) : : "rax", "rcx", "memory");
  if((result & 1u) == 0) {
    return false;
  }

  if (Cpu::HasSupport(CF_SSE) && Cpu::HasSupport(CF_SSE2)) {
    printf("\n SSE/SSE2 is supported.");
    __asm__ __volatile__("mov %%cr0, %%rax;"
                         "and $0xFFFB, %%ax;" // clear coprocessor emulation CR0.EM
                         "or $0x2, %%ax;" // set coprocessor monitoring CR0.MP
                         "mov %%rax, %%cr0;"
                         "mov %%cr4, %%rax;"
                         "or $0x600, %%ax;" // set CR4.OSFXSR and CR4.OSXMMEXCPT
                         "mov %%rax, %%cr4;" : : : );
    printf("\n SSE/SSE2 enabled");

    if (Cpu::HasSupport(CF_SSE3))
      printf("\n SSE3 is supported");

    if (Cpu::HasSupport(CF_SSE41))
      printf("\n SSE41 supported");

    if (Cpu::HasSupport(CF_SSE42))
      printf("\n SSE42 supported");

    if (Cpu::HasSupport(CF_SSSE3))
      printf("\n SSSE3 supported");

    if(Cpu::HasSupport(CF_FPU)) {
      //set NE flag
      __asm__ __volatile__("mov %%cr0, %%rax;"
                           "or $0x20, %%ax;"
                           "mov %%rax, %%cr0;" : : : );
      printf("\n Builtin FPU found");
    }
  } else {
    printf("\n SSE/SSE2 is not supported!!");
    return false;
  }
  return true;
}

void Processor::LIDT() {
  DTRegister IDTR;
  IDTR._limit = IDT::MAX_IDT_ENTRIES * sizeof(IDT::IDTEntry) - 1;
  IDTR._base = IDT_BASE_ADDR;

  __asm__ __volatile__("LIDT (%0)" : : "r"(&IDTR) : "memory");
  printf("\n IDT loaded for processor: %d", _id);
}

void Processor::LTR() {
  memset(_tss, 0, sizeof(TaskState64));
  _tss->_ioMapBase = 103;
  _tss->_rsp0 = MEM_KERNEL_RING0_STACK_TOP - _seqId * MAX_KERNEL_STACK_SIZE;
  _tss->_ist1 = MEM_KERNEL_IST1_TIMER_STACK_TOP - _seqId * MAX_KERNEL_STACK_SIZE;
  _tss->_ist2 = MEM_KERNEL_IST2_PAGE_FAULT_STACK_TOP - _seqId * MAX_KERNEL_STACK_SIZE;;
  _tss->_ist3 = MEM_KERNEL_IST3_XHCI_STACK_TOP - _seqId * MAX_KERNEL_STACK_SIZE;;
  _tss->_ist4 = MEM_KERNEL_IST4_COMMON_STACK_TOP - _seqId * MAX_KERNEL_STACK_SIZE;;

  __asm__ __volatile__("mov %0, %%ax;"
                       "ltr %%ax;" : : "m"(SYS_TSS_SELECTOR) :);
}

void Processor::incTimerTick() {
  __atomic_fetch_add(&_timerTicks, uint64_t(1), __ATOMIC_RELAXED);
}

uint64_t Processor::timerTicks() const {
  __atomic_load_n(&_timerTicks, __ATOMIC_RELAXED);
  return _timerTicks;
}