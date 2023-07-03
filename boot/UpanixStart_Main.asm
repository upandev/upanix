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

[GLOBAL PML4_TABLE]
[GLOBAL PML4_SIZE]
[GLOBAL PDP_TABLE]
[GLOBAL PDP_SIZE]
[GLOBAL PD_TABLE]
[GLOBAL PD_SIZE]
[GLOBAL PT_TABLE]
[GLOBAL PT_SIZE]

KERNEL_STACK_TOP EQU 8 * 1024 * 1024
MB_MAGIC EQU 0xE85250D6
MB_ARCH EQU 0

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
;HEADER_ADDR		DD		UPANIX_KERNEL_BASE_ADDRESS + MULTIBOOT_HEADER
;LOAD_ADDR		DD		UPANIX_KERNEL_BASE_ADDRESS
;LOAD_END_ADDR	DD		0
;BSS_END_ADDR	DD		0
;ENTRY_ADDR		DD		UPANIX_KERNEL_BASE_ADDRESS + _start

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[GLOBAL GDT]
[GLOBAL LDT_BASE_ADDR]
[GLOBAL IDT_BASE_ADDR]
[GLOBAL SYS_TSS_BASE_ADDR]
[GLOBAL USER_TSS_BASE_ADDR]

[GLOBAL _bootstrap]

[GLOBAL GLOBAL_DATA_SEGMENT_BASE]
[GLOBAL SYS_CODE_SELECTOR]
[GLOBAL SYS_LINEAR_SELECTOR]
[GLOBAL SYS_DATA_SELECTOR]
[GLOBAL SYS_TSS_SELECTOR]
[GLOBAL USER_TSS_SELECTOR]
[GLOBAL INT_TSS_SELECTOR_SV]
[GLOBAL INT_TSS_SELECTOR_PF]
[GLOBAL CALL_GATE_SELECTOR]
[GLOBAL INT_GATE_SELECTOR]
[GLOBAL MULTIBOOT2_INFO_ADDR]
[GLOBAL MULTIBOOT2_BOOTLOADER_MAGIC_VAL]
[GLOBAL CR0_CONTENT]
[GLOBAL CO_PROC_FPU_TYPE]

[EXTERN FPU_INIT]
[EXTERN UpanixMain]

PAGE_SIZE EQU 4096

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

    ; initial mapping up to 256 MB
    ; PML4 first entry
    MOV EDI, [PML4_TABLE]
    MOV EAX, [PDP_TABLE]
    OR EAX, 0x3 ; 1->writable, 1->present
    MOV DWORD [EDI], EAX
    MOV DWORD [EDI + 4], 0

    ; PDP first entry
    MOV EDI, [PDP_TABLE]
    MOV EAX, [PD_TABLE]
    OR EAX, 0x3 ; 1->writable, 1->present
    MOV DWORD [EDI], EAX
    MOV DWORD [EDI + 4], 0

    ; PD 128 entries. Each entry is 2MB * 128 -> 256 MB
    MOV ECX, 128
    MOV EDI, [PD_TABLE]
    MOV EAX, [PT_TABLE]
    OR EAX, 0x3 ; 1->writable, 1->present
    init_pd_loop:
      MOV DWORD [EDI], EAX
      MOV DWORD [EDI + 4], 0
      ADD EDI, 8
      ADD EAX, PAGE_SIZE
      loop init_pd_loop

    ; PT 128 entries. Each entry is 4KB * 128 * 512 -> 256 MB
    MOV ECX, 128 * 512
    MOV EDI, [PT_TABLE]
    MOV EAX, 0x0
    OR EAX, 0x3 ; 1->writable, 1->present
    init_pt_loop:
      MOV DWORD [EDI], EAX
      MOV DWORD [EDI + 4], 0
      ADD EDI, 8
      ADD EAX, PAGE_SIZE
      loop init_pt_loop

    ; init CR3 with PML4
    MOV EAX, [PML4_TABLE]
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

    ; load gdt
    LGDT [GDT.POINTER]

    JMP (GDT.CODE):(_MosMain)

[BITS 64]
SECTION .text
_MosMain:
    CLI
    CALL UpanixMain
    HLT

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; LOAD UPANIX KERNEL TO 1MB LOCATION IN RAM
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;LGDT [GDTR]

	;JMP SYS_CODE_SEL:_MosMain

;	MOV AX, SYS_DATA_SEL
;	MOV DS, AX
;	MOV SS, AX
;	MOV FS, AX
;	MOV GS, AX
;	MOV ESP, KERNEL_STACK
;	MOV AX, SYS_LINEAR_SEL
;	MOV ES, AX
;
;	MOV DWORD [GLOBAL_DATA_SEGMENT_BASE], UPANIX_KERNEL_BASE_ADDRESS
;
;	MOV WORD [SYS_CODE_SELECTOR], SYS_CODE_SEL
;	MOV WORD [SYS_LINEAR_SELECTOR], SYS_LINEAR_SEL
;	MOV WORD [SYS_DATA_SELECTOR], SYS_DATA_SEL
;	MOV WORD [SYS_TSS_SELECTOR], SYS_TSS_SEL
;	MOV WORD [USER_TSS_SELECTOR], USER_TSS_SEL
;	MOV WORD [INT_TSS_SELECTOR_SV], INT_TSS_SEL_SV
;	MOV WORD [INT_TSS_SELECTOR_PF], INT_TSS_SEL_PF
;	MOV WORD [CALL_GATE_SELECTOR], CALL_GATE_SEL
;	MOV WORD [INT_GATE_SELECTOR], INT_GATE_SEL
;
;   MOV DWORD [GDT_BASE_ADDR], GDT_BASE
;   MOV DWORD [LDT_BASE_ADDR], LDT_BASE
;   MOV DWORD [IDT_BASE_ADDR], IDT_BASE
;	MOV DWORD [SYS_TSS_BASE_ADDR], SYS_TSS_BASE
;	MOV DWORD [USER_TSS_BASE_ADDR], USER_TSS_BASE
;
;	MOV EAX,CR0
;	MOV DWORD [CR0_CONTENT], EAX
;
;	;CALL FPU_INIT
;
;	MOV AX, SYS_TSS_SEL
;	LTR AX
;
;	;***********************************
;	CALL UpanixMain
;	;***********************************

	HLT

SECTION .bss
ALIGN 4096
LDT_BASE: RESB 8192
IDT_BASE: RESB 4096
SYS_TSS_BASE: RESB 4096
USER_TSS_BASE: RESB 4096
INT_TSS_BASE_SV: RESB 4096
INT_TSS_BASE_PF: RESB 4096

LDT_BASE_ADDR: RESD 1
IDT_BASE_ADDR: RESD 1
SYS_TSS_BASE_ADDR: RESD 1
USER_TSS_BASE_ADDR: RESD 1

SECTION .data
GLOBAL_DATA_SEGMENT_BASE:
		DD 0
SYS_CODE_SELECTOR:
		DD 0
SYS_LINEAR_SELECTOR:
		DD 0
SYS_DATA_SELECTOR:
		DD 0
SYS_TSS_SELECTOR:
		DD 0
USER_TSS_SELECTOR:
		DD 0
INT_TSS_SELECTOR_SV:
		DD 0
INT_TSS_SELECTOR_PF:
		DD 0
CALL_GATE_SELECTOR:
		DD 0
INT_GATE_SELECTOR:
		DD 0
CR0_CONTENT:
		DD 0
CO_PROC_FPU_TYPE:
		DB 0

MULTIBOOT2_INFO_ADDR: DQ 0
MULTIBOOT2_BOOTLOADER_MAGIC_VAL: DD 0

; reserve page table space to map max 16GB RAM
; page mapping level-4 table -> 1 * 4K page
PML4_TABLE: DQ 0x1000000
PML4_SIZE: DD 512

PDP_TABLE: DQ 0x1001000
PDP_SIZE: DD 512

PD_TABLE: DQ 0x1002000
PD_SIZE: DD 8 * 1024

PT_TABLE: DQ 0x1012000
PT_SIZE: DD 4 * 1024 * 1024

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
        dq (1 <<44) | (1 << 47) | (1 << 41) | (1 << 43) | (1 << 53)  ;second entry=code=8
GDT:
  .NULL EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 0			; TYPE
    DB 0			; LIMIT 19:16, FLAGS
    DB 0			; BASE 31:24

  .CODE EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 10011010b    ; TYPE (1->present, 00->dpl, 1->code/data, 1->executable, 0, 1->readable, 0)
    DB 00100000b    ; (0, 0, 1->64bit, 0, 0000->limit 19:16)
    DB 0			; BASE 31:24

  .DATA EQU $ - GDT
    DW 0			; LIMIT 15:0
    DW 0			; BASE 15:0
    DB 0			; BASE 23:16
    DB 10010010b    ; TYPE (1->present, 00->dpl, 1->code/data, 0->data, 0, 1->read/write, 0)
    DB 0			; LIMIT 19:16, FLAGS
    DB 0			; BASE 31:24

  .POINTER:
    DW .POINTER - GDT - 1 ; size
	DQ GDT