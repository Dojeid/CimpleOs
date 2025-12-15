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