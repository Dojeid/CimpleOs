; 64-bit GDT flush function

section .text
bits 64

global gdt_flush

gdt_flush:
    ; RDI contains pointer to GDT pointer struct (System V AMD64 ABI)
    lgdt [rdi]
    
    ; Reload segments
    mov ax, 0x10    ; Data segment (offset 0x10 in GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far return to reload CS
    pop rdi         ; Save return address
    mov rax, 0x08   ; Code segment (offset 0x08 in GDT)
    push rax
    push rdi
    retfq           ; 64-bit far return
