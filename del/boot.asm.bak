; src/arch/i386/boot.asm

; --- Multiboot Header Constants ---
MBALIGN  equ  1 << 0            ; Align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; Provide memory map
MBVIDEO  equ  1 << 2            ; Ask for Video Mode
FLAGS    equ  MBALIGN | MEMINFO | MBVIDEO
MAGIC    equ  0x1BADB002        ; 'Magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; Checksum required

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; --- Header Address Fields (Required if flags[16] is set, we zero them) ---
    dd 0, 0, 0, 0, 0

    ; --- Video Mode Preference ---
    ; 0 = Linear Graphics
    ; Width = 1024, Height = 768, Depth = 32-bit (Colors)
    dd 0
    dd 1024
    dd 768
    dd 32

section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB
stack_top:

section .text
global start
extern kmain

start:
    ; Set up the stack
    mov esp, stack_top

    ; GRUB puts the address of the Multiboot Info Struct into EBX.
    ; We must push this onto the stack so our C kernel can read it!
    push ebx

    ; Call the C kernel
    call kmain

    ; Halt the CPU if kernel returns
    cli
.hang:
    hlt
    jmp .hang

; --- GDT Flush Routine (This was missing!) ---
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]  ; Get the pointer to the GDT pointer (passed as argument)
    lgdt [eax]          ; Load the GDT

    ; Reload data segment registers
    mov ax, 0x10        ; 0x10 is offset to data segment in GDT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload code segment (0x08 is offset to code segment)
    jmp 0x08:.flush_done

.flush_done:
    ret

; Fixes the linker warning about executable stack
section .note.GNU-stack noalloc noexec nowrite progbits