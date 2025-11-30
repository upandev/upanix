;	Upanix - An x86 based Operating System
;	Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
;
; I am making my contributions/submissions to this project solely in
; my personal capacity and am not conveying any rights to any
; intellectual property of any third parties.
;	                                                                        
;	This program is free software: you can redistribute it and/or modify
;	it under the terms of the GNU General Public License as published by
;	the Free Software Foundation, either version 3 of the License, or
;	(at your option) any later version.
;	                                                                        
;	This program is distributed in the hope that it will be useful,
;	but WITHOUT ANY WARRANTY; without even the implied warranty of
;	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;	GNU General Public License for more details.
;	                                                                        
;	You should have received a copy of the GNU General Public License
;	along with this program.  If not, see <http://www.gnu.org/licenses/

[BITS 32]

[GLOBAL MEM_PML4_TABLE]
[GLOBAL MEM_PML4_SIZE]
[GLOBAL MEM_PDP_TABLE]
[GLOBAL MEM_PDP_SIZE]
[GLOBAL MEM_PD_TABLE]
[GLOBAL MEM_PD_SIZE]
[GLOBAL MEM_PT_TABLE]
[GLOBAL MEM_PT_SIZE]
[GLOBAL MEM_INIT_PAGE_MAP_SIZE]

KERNEL_STACK_TOP EQU 32 * 1024 * 1024
MB_MAGIC EQU 0xE85250D6
MB_ARCH EQU 0

PAGE_SIZE EQU 4096

;do we need to define bootstrap stack ??

SECTION .multiboot
ALIGN 8
MULTIBOOT_HEADER:
MAGIC:          DD MB_MAGIC
ARCH:           DD MB_ARCH
HEADER_LEN:     DD (MULTIBOOT_HEADER_END - MULTIBOOT_HEADER)
CHECKSUM:       DD -(MB_MAGIC + MB_ARCH + (MULTIBOOT_HEADER_END - MULTIBOOT_HEADER))

FRAMEBUFFER_TAG:
  DW  0x05 ;type = 5 -> Framebuffer
  DW  0x01 ;optional tag
  DD  FRAMEBUFFER_TAG_END - FRAMEBUFFER_TAG ;size
  DD  0   ;width - 0 means bootloader will decide
  DD  0   ;height
  DD  0   ;depth
FRAMEBUFFER_TAG_END:

ALIGN 8
END_TAG:
  DW 0
  DW 0
  DD 8

MULTIBOOT_HEADER_END:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[GLOBAL GDT]
[GLOBAL LDT_BASE_ADDR]
[GLOBAL IDT_BASE_ADDR]
[GLOBAL TSS_BASE_ADDR]

[GLOBAL _bootstrap]

[GLOBAL SYS_CODE_SELECTOR]
[GLOBAL SYS_DATA_SELECTOR]
[GLOBAL USER_CODE_SELECTOR]
[GLOBAL USER_DATA_SELECTOR]

[GLOBAL SYS_TSS_SELECTOR]
[GLOBAL CALL_GATE_SELECTOR]
[GLOBAL MULTIBOOT2_INFO_ADDR]
[GLOBAL MULTIBOOT2_BOOTLOADER_MAGIC_VAL]
[GLOBAL CR0_CONTENT]
[GLOBAL CO_PROC_FPU_TYPE]

[EXTERN FPU_INIT]
[EXTERN UpanixMain]

[BITS 32]
SECTION .text
_bootstrap:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Save the address of multiboot header in a variable MULTIBOOT2_INFO_ADDR
	; Upanix kernel will copy the contents first thing when control enters UpanixMain in Multiboot constructor
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MOV [MULTIBOOT2_INFO_ADDR], EBX
	MOV [MULTIBOOT2_BOOTLOADER_MAGIC_VAL], EAX
	MOV ESP, KERNEL_STACK_TOP

	MOV AL, 11111111B               ; SELECT TO MASK OF ALL IRQ'S
	OUT 0x21, AL                    ; WRITE IT TO THE PIC CONTROLLER


    MOV AL, 0
    MOV ECX, 4096
    MOV EDI, IDT_BASE
    REP STOSB
    MOV EDI, TSS_BASE
    REP STOSB

    ; initial mapping up to 256 MB
    ; PML4 first entry
    MOV EDI, [MEM_PML4_TABLE]
    MOV EAX, [MEM_PDP_TABLE]
    OR EAX, 0x3 ; 1->writable, 1->present
    MOV DWORD [EDI], EAX
    MOV DWORD [EDI + 4], 0

    ; PDP first entry
    MOV EDI, [MEM_PDP_TABLE]
    MOV EAX, [MEM_PD_TABLE]
    OR EAX, 0x3 ; 1->writable, 1->present
    MOV DWORD [EDI], EAX
    MOV DWORD [EDI + 4], 0

    ; PD 128 entries. Each entry is 2MB * 128 -> 256 MB
    MOV ECX, 128
    MOV EDI, [MEM_PD_TABLE]
    MOV EAX, [MEM_PT_TABLE]
    OR EAX, 0x3 ; 1->writable, 1->present
    init_pd_loop:
      MOV DWORD [EDI], EAX
      MOV DWORD [EDI + 4], 0
      ADD EDI, 8
      ADD EAX, PAGE_SIZE
      loop init_pd_loop

    ; PT 128 entries. Each entry is 4KB * 128 * 512 -> 256 MB
    MOV ECX, 128 * 512
    MOV EDI, [MEM_PT_TABLE]
    MOV EAX, 0x0
    OR EAX, 0x3 ; 1->writable, 1->present
    init_pt_loop:
      MOV DWORD [EDI], EAX
      MOV DWORD [EDI + 4], 0
      ADD EDI, 8
      ADD EAX, PAGE_SIZE
      loop init_pt_loop

    ; init CR3 with PML4
    MOV EAX, [MEM_PML4_TABLE]
    MOV CR3, EAX

    ; set PAE
    MOV EAX, CR4
    OR EAX, 1 << 5
    MOV CR4, EAX

    ; Now set up the long mode bit
    MOV ECX, 0xC0000080
    ; copy the values of msr into eax
    RDMSR
    OR EAX, 1 << 8
    ; write back the value
    WRMSR

    ; enable paging
    MOV EAX, CR0
    OR EAX, 1 << 31 ; Paging bit
    OR EAX, 1 << 16 ; Write protect, cpu  can't write to read-only pages when
                    ; privilege level is 0
    MOV CR0, EAX    ; write back cr0


    MOV EAX, TSS_BASE
	MOV WORD [GDT.GDT_TSS + 2], AX
	SHR EAX, 16
	MOV BYTE [GDT.GDT_TSS + 4], AL
	MOV BYTE [GDT.GDT_TSS + 7], AH

    ; load gdt
    LGDT [GDT.POINTER]

    JMP (GDT.SYS_CODE):(_MosMain)

[BITS 64]
SECTION .text
_MosMain:
    CLI

    MOV RSP, KERNEL_STACK_TOP

    MOV WORD [SYS_CODE_SELECTOR], GDT.SYS_CODE
    MOV WORD [SYS_DATA_SELECTOR], GDT.SYS_DATA

    MOV AX, GDT.SYS_DATA
    MOV SS, AX
    MOV DS, AX
    MOV ES, AX
    MOV FS, AX
    MOV GS, AX

    MOV WORD [USER_CODE_SELECTOR], GDT.USER_CODE
    MOV WORD [USER_DATA_SELECTOR], GDT.USER_DATA

    MOV QWORD [IDT_BASE_ADDR], IDT_BASE

    MOV WORD [SYS_TSS_SELECTOR], GDT.TSS
    MOV QWORD [TSS_BASE_ADDR], TSS_BASE

    CALL FPU_INIT

    CALL UpanixMain

    HLT

SECTION .bss
ALIGN 4096
LDT_BASE: RESB 8192
IDT_BASE: RESB 4096
TSS_BASE: RESB 512

IDT_BASE_ADDR: RESQ 1
LDT_BASE_ADDR: RESD 1
TSS_BASE_ADDR: RESQ 1

SECTION .data
SYS_CODE_SELECTOR:
		DW 0
SYS_DATA_SELECTOR:
		DW 0
USER_CODE_SELECTOR:
        DW 0
USER_DATA_SELECTOR:
        DW 0
SYS_TSS_SELECTOR:
		DD 0
CALL_GATE_SELECTOR:
		DD 0
CR0_CONTENT:
		DD 0
CO_PROC_FPU_TYPE:
		DB 0

MULTIBOOT2_INFO_ADDR: DQ 0
MULTIBOOT2_BOOTLOADER_MAGIC_VAL: DD 0

; reserve page table space to map max 16GB RAM
; page mapping level-4 table -> 1 * 4K page
MEM_PML4_TABLE: DQ 0x2000000
MEM_PML4_SIZE: DD 512

MEM_PDP_TABLE: DQ 0x2001000
MEM_PDP_SIZE: DD 512

MEM_PD_TABLE: DQ 0x2002000
MEM_PD_SIZE: DD 8 * 1024

MEM_PT_TABLE: DQ 0x2012000
MEM_PT_SIZE: DD 4 * 1024 * 1024

MEM_INIT_PAGE_MAP_SIZE: DD 256 * 1024 * 1024

SECTION .rodata
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	GLOBAL DESCRIPTOR TABLE (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ALIGN 4096
; set the following values:
; descriptor type: bit 44 has to be 1 for code and data segments
; present: bit 47 has to be  1 if the entry is valid
; read/write: bit 41 1 means that is readable
; executable: bit 43 it has to be 1 for code segments
; 64bit: bit 53 1 if this is a 64bit gdt
;dq (1 <<44) | (1 << 47) | (1 << 41) | (1 << 43) | (1 << 53)  ;second entry=code=8

GDT:
  .NULL EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 0			; TYPE
    DB 0			; LIMIT 19:16, FLAGS
    DB 0			; BASE 31:24

  .SYS_CODE EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 10011010b    ; TYPE (1->present, 00->dpl, 1->code/data, 1->executable, 0, 1->readable, 0)
    DB 00100000b    ; (0, 0, 1->64bit, 0, 0000->limit 19:16)
    DB 0			; BASE 31:24

  .SYS_DATA EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 10010010b    ; TYPE (1->present, 00->dpl, 1->code/data, 0->data, 0, 1->read/write, 0)
    DB 0			; LIMIT 19:16, FLAGS
    DB 0			; BASE 31:24

  .USER_DATA EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 11110010b    ; TYPE (1->present, 11->dpl, 1->code/data, 0->data, 0, 1->read/write, 0)
    DB 0			; LIMIT 19:16, FLAGS
    DB 0			; BASE 31:24

  .USER_CODE EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 11111010b    ; TYPE (1->present, 11->dpl, 1->code/data, 1->executable, 0, 1->readable, 0)
    DB 00100000b    ; (0, 0, 1->64bit, 0, 0000->limit 19:16)
    DB 0			; BASE 31:24

  .TSS EQU $ - GDT
  .GDT_TSS:
	DW 103          ; LIMIT 15:0
	DW 0			; BASE 15:0, SET ABOVE
	DB 0            ; BASE 23:16, SET ABOVE
	DB 10001001b    ; PRESENT, RING 0, 64-BIT AVAILABLE TSS
	DB 0            ; LIMIT 19:16, FLAGS
	DB 0            ; BASE 31:24, SET ABOVE
	DD 0            ; BASE 63:33, SET ABOVE
	DD 0            ; RESERVED

  .POINTER:
    DW .POINTER - GDT; size
	DQ GDT
