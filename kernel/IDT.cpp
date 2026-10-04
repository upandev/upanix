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
#include <IDT.h>
#include <MemConstants.h>
#include <MemManager.h>
#include <PIT.h>
#include <UpanixMain.h>
#include <InterruptHandlers.h>

IDT::IDT() {
  const int MAX_IDT_ENTRIES = 50;
  for (unsigned i = 0; i < MAX_IDT_ENTRIES; ++i) {
    LoadEntry(i, (uintptr_t) &isr_default_interrupt_handler, SYS_CODE_SELECTOR, 0x8E);
  }

  LoadHandlers() ;

	IDT::IDTRegister IDTR ;

	IDTR._limit = MAX_IDT_ENTRIES * sizeof(IDT::IDTEntry) ;
	IDTR._base = IDT_BASE_ADDR ;

	__asm__ __volatile__("LIDT (%0)" : : "r"(&IDTR)) ;
  KC::MConsole().LoadMessage("IDT Initialization", Success) ;
}

extern "C" {
  void _page_fault_interrupt_handler();
  void _isr_0x27_interrupt_handler();
  void _timer_interrupt_handler();
  void _xhci_interrupt_handler();
}

void IDT::LoadHandlers() {
	LoadEntry(0, (uintptr_t)&isr_0_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(1, (uintptr_t)&isr_1_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(2, (uintptr_t)&isr_2_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(3, (uintptr_t)&isr_3_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(4, (uintptr_t)&isr_4_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(5, (uintptr_t)&isr_5_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(6, (uintptr_t)&isr_6_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
  LoadEntry(7, (uintptr_t)&isr_7_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(8, (uintptr_t)&isr_8_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(9, (uintptr_t)&isr_9_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(10, (uintptr_t)&isr_10_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(11, (uintptr_t)&isr_11_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(12, (uintptr_t)&isr_12_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(13, (uintptr_t)&isr_13_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
	LoadEntry(14, (uintptr_t)&_page_fault_interrupt_handler, SYS_CODE_SELECTOR, 0xEE) ;
	LoadEntry(16, (uintptr_t)&isr_16_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
  LoadEntry(17, (uintptr_t)&isr_17_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
  LoadEntry(18, (uintptr_t)&isr_18_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
  LoadEntry(19, (uintptr_t)&isr_19_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
  LoadEntry(20, (uintptr_t)&isr_20_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
  LoadEntry(21, (uintptr_t)&isr_21_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;

	//Spurious IRQ
	LoadEntry(0x27, (uintptr_t)&_isr_0x27_interrupt_handler, SYS_CODE_SELECTOR, 0x8E) ;
}

void IDT::LoadEntry(uint32_t idtNo, uintptr_t offset, uint16_t selector, uint8_t options) {
	IDT::IDTEntry* idtEntry = reinterpret_cast<IDT::IDTEntry*>(IDT_BASE_ADDR) + idtNo;
	
	idtEntry->_lowerOffset = offset & 0xFFFF;
  idtEntry->_midOffset = (offset >> 16) & 0xFFFF;
  idtEntry->_higherOffset = (offset >> 32) & 0xFFFFFFFF;
	idtEntry->_selector = selector;
	idtEntry->_reserved = 0;
	idtEntry->_options = options;

	//we need to keep the stack separate for timer (int 0x20) and page-fault because page-fault handler
	//delegates page allocation to kernel-service and yields by calling int 0x20
	//This leads to a nested interrupt/exception scenario, and hence they both must use a different stack frame
	//In general, if any interrupt or exception invokes another interrupt, then we need to ensure they both use different stack frame
	if (offset == (uintptr_t)&_timer_interrupt_handler) {
    idtEntry->_ist = 1;
	}	else if (offset == (uintptr_t)&_page_fault_interrupt_handler) {
    idtEntry->_ist = 2;
  }	else if (offset == (uintptr_t)&_xhci_interrupt_handler) {
    idtEntry->_ist = 3;
	} else {
    idtEntry->_ist = 4;
	}
}