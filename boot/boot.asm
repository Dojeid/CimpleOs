; Declare constants for the multiboot header.
MBALIGN  equ  1<<0            ; align loaded modules on page boundaries
MEMINFO  equ  1<<1            ; provide memory map
FLAGS    equ  MBALIGN | MEMINFO ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002      ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS) ; checksum required

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Reserve a stack for the kernel.
section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB
stack_top:

section .text
global start
extern kmain

start:
    ; Set up the stack.
    mov esp, stack_top

    ; Call the C kernel's main function.
    call kmain

    ; Halt the CPU after the kernel returns.
    cli
.hang:
    hlt
    jmp .hang

; Add this section to tell the linker that our stack is not executable.
section .note.GNU-stack noalloc noexec nowrite progbits