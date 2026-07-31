; Falkon-OS 64-bit Bootloader
; Multiboot 1 compliant header — long mode entry with 4-level page tables

MBALIGN  equ  1 << 0    ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1    ; provide memory map
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

align 16
bits 32
global start
extern kmain

start:
    mov esp, stack_top
    push ebx
    
    call check_cpuid
    call check_long_mode
    call setup_page_tables
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Load PML4
    mov eax, pml4_table
    mov cr3, eax
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, "C"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, "L"
    jmp error

setup_page_tables:
    mov edi, pml4_table
    mov ecx, 4096
    xor eax, eax
    rep stosd
    
    ; PML4[0] -> PDPT
    mov eax, pdpt_table
    or eax, 0b11
    mov [pml4_table], eax
    
    ; PDPT[0] -> PD
    mov eax, pd_table
    or eax, 0b11
    mov [pdpt_table], eax
    
    ; Identity-map 64MB using 2MB huge pages (32 PD entries).
    ; Required for the kernel heap at 0x1000000 (16MB-32MB) and future drivers.
    mov edi, pd_table
    mov eax, 0x00000083     ; Present | R/W | PS (2MB page)
    mov ecx, 32             ; 32 * 2MB = 64MB
.map_pd:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_pd
    
    ret

error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov byte [0xb8008], al
    hlt

section .rodata
align 8
gdt64:
    dq 0                        ; Null Descriptor
.code: equ $ - gdt64
    dq 0x00209A0000000000       ; 64-bit Code Segment (L=1, D=0, P=1, S=1, C/R=1)
.data: equ $ - gdt64
    dq 0x0000920000000000       ; 64-bit Data Segment (P=1, S=1, W=1)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 16
stack_bottom:
resb 32768
stack_top:

align 4096
pml4_table:
    resb 4096
pdpt_table:
    resb 4096
pd_table:
    resb 4096

section .text
bits 64
long_mode_start:
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov rsp, stack_top
    pop rdi
    
    call kmain
    
    cli
.hang:
    hlt
    jmp .hang
