; This macro creates a stub for an ISR (Interrupt Service Routine)
; It saves the processor state, calls C code, and restores state.

global isr1
extern keyboard_handler ; We will write this C function in Step 4

isr1:
    cli                 ; Disable interrupts
    pusha               ; Push all general purpose registers (eax, ebx, ecx...)
    
    call keyboard_handler ; Call our C driver
    
    popa                ; Restore all registers
    sti                 ; Re-enable interrupts
    iret                ; Return from interrupt (Resume what we were doing)
    ; Add this below isr1
    global isr12
    extern mouse_handler
    isr12:
        cli
        pusha
        call mouse_handler
        popa
        sti
        iret
section .note.GNU-stack noalloc noexec nowrite progbits