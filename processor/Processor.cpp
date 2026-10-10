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
#include <TscClock.h>
#include <DMM.h>

extern "C" {
  extern const uint8_t AP_TRAMPOLINE_START[];

  extern uint8_t BSP_GDT[];
  extern uint16_t BSP_GDT_SIZE;

  extern uint32_t AP_EFER_BITS;
  extern uint64_t AP_STACK_TOP;

  Processor::DTRegister AP_GDTR;
  static uint32_t AP_REPORTED_ID;
  static uint32_t AP_READY;
}

Processor::Processor(int seqId, uint32_t id) : _seqId(seqId), _id(id), _tss(nullptr) {
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

BootstrapProcessor::BootstrapProcessor(int seqId, uint32_t id) : Processor(seqId, id) {
  AP_EFER_BITS = (uint32_t)(Cpu::Instance().MSRread(IA32_EFER) & (1ULL << 11));
  _tss = (TaskState64*)TSS_BASE_ADDR;

  //load LTR to ensure stack is setup in case if there is an exception after LIDT
  LTR();
  LIDT();
}

ApplicationProcessor::ApplicationProcessor(int seqId, uint32_t id, Apic& apic) : Processor(seqId, id), _apic(apic) {
  _gdtBase = KernelDMM::Instance().allocate(BSP_GDT_SIZE, 8);
  memcpy((void*)_gdtBase, BSP_GDT, BSP_GDT_SIZE);

  uintptr_t tssBase = MEM_KERNEL_TSS_START + _seqId * MAX_KERNEL_TSS_SIZE;
  _tss = (TaskState64*)tssBase;

  uintptr_t tssSegment = _gdtBase + SYS_TSS_SELECTOR;
  *(uint16_t*)(tssSegment + 2) = tssBase & 0xFFFF;
  *(uint8_t*)(tssSegment + 4) = (tssBase >> 16) & 0xFF;
  *(uint8_t*)(tssSegment + 7) = (tssBase >> 24) & 0xFF;
  *(uint32_t*)(tssSegment + 8) = (tssBase >> 32) & 0xFFFFFFFF;

  // Present, ring 0, available 64-bit TSS.
  *(uint8_t*)(tssSegment + 5) = 0x89;
}

void ApplicationProcessor::init() {
  printf("\nInitializing AP with APIC ID %u", _id);
  const auto startupVector = uint8_t(uintptr_t(AP_TRAMPOLINE_START) >> 12);

  AP_STACK_TOP = MEM_KERNEL_STACK_TOP - _seqId * MAX_KERNEL_STACK_SIZE;

  AP_GDTR._limit = BSP_GDT_SIZE - 1;
  AP_GDTR._base = _gdtBase;

  __atomic_store_n(&AP_REPORTED_ID, 0u, __ATOMIC_RELAXED);
  __atomic_store_n(&AP_READY, 0u, __ATOMIC_RELAXED);

  _apic.initAP(_id);
  TscClock::instance().busyWait(10000);

  bool apReady = false;
  _apic.sipiAP(_id, startupVector);
  for (int i = 0; i < 200 && !apReady; ++i) {
    TscClock::instance().busyWait(500);
    apReady = __atomic_load_n(&AP_READY, __ATOMIC_ACQUIRE) == 1;
  }

  if (!apReady) {
    printf("\n AP with APIC ID %u failed to initialize", _id);
  } else {
    const auto apId = __atomic_load_n(&AP_REPORTED_ID, __ATOMIC_ACQUIRE);
    printf("\n AP with APIC ID %u initialized, Reported ID: %u", _id, apId);
  }
}

void ApplicationProcessor::main() {
  LTR();
  LIDT();

  __atomic_store_n(&AP_REPORTED_ID, _apic.GetLocalApicID(), __ATOMIC_RELAXED);
  __atomic_store_n(&AP_READY, 1u, __ATOMIC_RELEASE);
}