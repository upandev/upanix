; AP startup code: linked and loaded into a reserved page at 0x8000.
; ELF64 object containing 16-bit, 32-bit, and 64-bit instructions.

BITS 16
SECTION .ap_init progbits alloc exec write
ALIGN 4096

GLOBAL AP_TRAMPOLINE_START
GLOBAL AP_TRAMPOLINE_END
GLOBAL AP_EFER_BITS
GLOBAL AP_STACK_TOP

EXTERN MEM_PML4_TABLE
EXTERN SYS_CODE_SELECTOR
EXTERN SYS_DATA_SELECTOR
EXTERN AP_GDTR
EXTERN _ap_main

AP_TRAMPOLINE_START:
    cli
    cld
    jmp short real_mode

; Mailbox fields filled by the BSP before sending SIPI.
times 0x10 - ($ - $$) db 0
AP_EFER_BITS:         dd 0 ; 0x14: BSP EFER.NXE (bit 11), or zero
AP_STACK_TOP:         dq 0 ; 0x18: private, mapped stack top

real_mode:
    xor ax, ax
    mov ds, ax
    mov es, ax

    lgdt [GDT.POINTER]

    mov eax, cr0
    or eax, 1                    ; CR0.PE
    mov cr0, eax

    jmp dword GDT.SYS_CODE32:protected_mode

BITS 32
protected_mode:
    mov ax, GDT.SYS_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov eax, cr4
    or eax, 1 << 5               ; CR4.PAE
    mov cr4, eax
    mov eax, [MEM_PML4_TABLE]
    mov cr3, eax

    mov ecx, 0xC0000080          ; IA32_EFER
    rdmsr
    mov esi, [AP_EFER_BITS]
    and esi, 1 << 11             ; allow only NXE from the BSP
    or eax, esi
    or eax, 1 << 8               ; EFER.LME
    wrmsr

    mov eax, cr0
    or eax, (1 << 31) | (1 << 16) ; CR0.PG and CR0.WP
    mov cr0, eax

    jmp GDT.SYS_CODE:long_mode

BITS 64
DEFAULT REL
long_mode:
    mov ax, GDT.SYS_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax

    ; load gdt
    LGDT [AP_GDTR]

    ;jmp SYS_CODE_SELECTOR:long_mode
    ;instead of simply doing above, we are calculating the far target like below because GDT.SYS_CODE and the SYS_CODE_SELECTOR may not be same
    ;there is no way to use SYS_CODE_SELECTOR variable in jmp directly

    mov ax, [SYS_CODE_SELECTOR]
    mov [start_ap_target_address + 8], ax

    lea rax, [rel _start_ap]
    mov [start_ap_target_address], rax

    jmp far qword [rel start_ap_target_address]

_start_ap:
    mov ax, [SYS_DATA_SELECTOR]
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    mov rsp, [AP_STACK_TOP]
    xor ebp, ebp

    call _ap_main

.park:
    hlt                          ; interrupts remain disabled
    jmp .park

ALIGN 8

start_ap_target_address:
    dq 0    ; 64-bit destination address
    dw 0    ; 16-bit code selector

GDT:
    .NULL EQU $ - GDT
        DW 0                ; Limit 15:0
        DW 0                ; Base 15:0
        DB 0                ; Base 23:16
        DB 0                ; Access
        DB 0                ; Flags and limit 19:16
        DB 0                ; Base 31:24

    .SYS_CODE32 EQU $ - GDT
        DW 0xFFFF           ; Limit 15:0
        DW 0                ; Base 15:0
        DB 0                ; Base 23:16
        DB 10011010b        ; Present, ring 0, executable, readable
        DB 11001111b        ; G=1, D=1, L=0, limit 19:16=0xF
        DB 0                ; Base 31:24

    .SYS_DATA EQU $ - GDT
        DW 0xFFFF           ; Limit 15:0
        DW 0                ; Base 15:0
        DB 0                ; Base 23:16
        DB 10010010b        ; Present, ring 0, writable data
        DB 11001111b        ; G=1, B=1, limit 19:16=0xF
        DB 0                ; Base 31:24

    .SYS_CODE EQU $ - GDT
        DW 0                ; Limit ignored in 64-bit mode
        DW 0                ; Base 15:0
        DB 0                ; Base 23:16
        DB 10011010b        ; Present, ring 0, executable, readable
        DB 00100000b        ; L=1, D=0: 64-bit code
        DB 0                ; Base 31:24

    .POINTER:
        DW .POINTER - GDT - 1   ; GDT limit: size minus one
        DD GDT              ; 32-bit base for our early LGDT

; Also makes NASM fail if the trampoline ever exceeds one startup page.
times 4096 - ($ - $$) db 0

AP_TRAMPOLINE_END: