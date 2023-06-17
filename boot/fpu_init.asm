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

[EXTERN CR0_CONTENT]
[EXTERN CO_PROC_FPU_TYPE]

[GLOBAL FPU_INIT]

SECTION .text

FPCW: DD 0x5A5A

NO_CO_PROC		EQU 1
CO_PROC_87_287	EQU 2
CO_PROC_387		EQU 3

FPU_INIT: 
    MOV EAX, CR0            ; eax = CR0
    AND AL, ~0x6            ; Clear the EM and MP flags (just in case)
    MOV CR0, EAX            ; Set CR0

	FNINIT					; Must use non-wait form

	FNSTSW [FPCW]			; Must use non-wait form of fstsw
							; It is not necessary to use a WAIT instruction
							;  after fnstsw or fnstcw.  Do not use one here.

	CMP BYTE [FPCW], 0	; See if correct status with zeroes was read
	JNE NO_NPX				; Jump if not a valid status word, meaning no NPX

							; Now see if ones can be correctly written from the control word.

	FNSTCW [FPCW]			; Look at the control word; do not use WAIT form
							; Do not use a WAIT instruction here!

	MOV AX, [FPCW]			; See if ones can be written by NPX
	AND AX, 0x103F			; See if selected parts of control word look OK
	CMP AX, 0x3F			; Check that ones and zeroes were correctly read
	JNE NO_NPX				; Jump if no NPX is installed

							; Some numerics chip is installed.  NPX instructions and WAIT are now safe.
							; See if the NPX is an 8087, 80287, or 80387.
							; This code is necessary if a denormal exception handler is used or the
							; new 80387 instructions will be used.

	FLD1					; Must use default control word from FNINIT
	FLDZ					; Form infinity
;	TFDIV: DD 0x9BDEF9
	FDIV st1, st0					; 8087/287 says +inf = .inf
;	TFLDST: DD 0x9BD9C0
	FLD st0					; Form negative infinity
	FCHS					; 80387 says +inf <> -inf
	FCOMPP					; See if they are the same and remove them
	FSTSW [FPCW]				; Look at status from FCOMPP

	MOV AX, [FPCW]
	SAHF					; See if the infinities matched
	JE FOUND_87_287			; Jump if 8087/287 is present

							; An 80387 is present.  If denormal exceptions are used for an 8087/287,
							; they must be masked.  The 80387 will automatically normalize denormal
							; operands faster than an exception handler can.

	JMP FOUND_387

NO_NPX:
							; set up for no NPX
	MOV BYTE [CO_PROC_FPU_TYPE], NO_CO_PROC
	JMP EXIT

FOUND_87_287:
							; set up for 87/287
	MOV BYTE [CO_PROC_FPU_TYPE], CO_PROC_87_287
	JMP FOUND

FOUND_387:
							; set up for 387
	MOV BYTE [CO_PROC_FPU_TYPE], CO_PROC_387
	JMP FOUND

FOUND:

	MOV EAX, CR0
	OR  EAX, 0x2 ; Set MP bit 2
	MOV CR0, EAX

	JMP EXIT

EXIT:
	RET
