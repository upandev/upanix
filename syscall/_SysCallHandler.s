#	Upanix - An x86 based Operating System
#	Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
#
# I am making my contributions/submissions to this project solely in
# my personal capacity and am not conveying any rights to any
# intellectual property of any third parties.
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/
.code64

.extern PROCESS_SYSCALL_RETURN_ADDRESS

.section .data

TEMP_RSP: .quad 0

.section .text

.macro _save_regs
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

.macro _restore_regs
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

.global _syscall_handler
.extern SysCall_Entry

_syscall_handler:
  movq %rsp, TEMP_RSP

  and $0xFFFFFFFFFFFFFFF0, %rsp #Clear the 4 least significant bits to align to 16-byte boundary
  sub $8, %rsp #Reserve additional 8 bytes for alignment

  _save_regs

  movq TEMP_RSP, %rax
  movq (%rax), %rdi
  movq 8(%rax), %rsi
  movq 16(%rax), %rdx
  movq 24(%rax), %rcx
  movq 32(%rax), %r8
  movq 40(%rax), %r9
  push TEMP_RSP

  sti
  call SysCall_Entry
  cli

  pop TEMP_RSP

  _restore_regs

  movq TEMP_RSP, %rsp

  movq PROCESS_SYSCALL_RETURN_ADDRESS, %rax
  movq (%rax), %rax

  sysretq

.global _runtime_dll_resolver
.global _runtime_dll_resolver_end
_runtime_dll_resolver:
# Two Double Words (8 bytes) are already pushed onto
# Stack by Dynamic Relocation Process which are Second Entry GOT
# and Relocation Offset. These are sent as Arg 4 and Arg 5 for SysCall
  pushq $3
  push %r11 #need to save rcx and r11 as they are modifed by syscall (rcx = return rip, r11 = rflags)
  push %rcx
  pushq $601

  syscall

  #rcx is the 4th param of any function. So, it's important we save and restore it after syscall
  #now the restored rcx value will be the 4th param of the relocated function that is being jumped into by jmp *%rax
  movq 8(%rsp), %rcx
  movq 16(%rsp), %r11

  add $48, %rsp

  jmp *%rax

  ret

_runtime_dll_resolver_end:
