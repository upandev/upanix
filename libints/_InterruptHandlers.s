//	Upanix - An x86 based Operating System
//  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
//
//  I am making my contributions/submissions to this project solely in
//  my personal capacity and am not conveying any rights to any
//  intellectual property of any third parties.
//	                                                                        
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//	                                                                        
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//	                                                                        
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/

.code64
.section .text

.macro _save_interrupt_regs
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    subq $512, %rsp
    #As per Intel manuals, when TS flag is set and EM is clear then SSE instructions will cause GP
    #But in Qemu, this didn't cause any GP but I am doing it just to go by the doc
    clts
    #sse pointer on stack must be 16 byte aligned otherwise it will cause General Protection fault
    fxsave (%rsp)
.endm

.macro _restore_interrupt_regs
    clts
    fxrstor (%rsp)
    addq $512, %rsp
    
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
.endm

.macro interrupt_handler name:req, ec=0
.global _\name\()_interrupt_handler
.extern \name\()_interrupt_handler

_\name\()_interrupt_handler:
  .if \ec == 1
    #if an ec is on the stack, then, rsp is not 16 byte aligned
    #for now, discard the error code
    addq $8, %rsp
  .endif

  _save_interrupt_regs

  mov %rsp, %rdi //set the TaskContext param
  cld //clear direction flag
  call \name\()_interrupt_handler

  _restore_interrupt_regs

  iretq
.endm

interrupt_handler timer
interrupt_handler pit_timer
interrupt_handler keyboard
interrupt_handler mouse
interrupt_handler rtc
interrupt_handler xhci
interrupt_handler page_fault, 1
interrupt_handler isr_0x27
interrupt_handler ata_primary
interrupt_handler ata_secondary
interrupt_handler floppy
interrupt_handler e1000_nic
