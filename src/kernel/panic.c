#include "panic.h"
#include "drivers/video/graphics.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

// Exception messages
const char* exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Detailed panic screen
void kernel_panic(const char* message, uint32_t error_code) {
    // Disable interrupts
    asm volatile("cli");
    
    // Red screen of death
    clear_screen(0x990000);
    
    // Title
    draw_string(10, 10, 0xFFFFFF, "=== KERNEL PANIC ===");
    
    // Error message
    char buf[256];
    sprintf(buf, "Fatal Error: %s", message);
    draw_string(10, 30, 0xFFFF00, buf);
    
    // Error code if present
    if (error_code != 0) {
        sprintf(buf, "Error Code: 0x%X", error_code);
        draw_string(10, 50, 0xFFAAAA, buf);
    }
    
    // Instructions
    draw_string(10, 80, 0xAAAAAA, "The system has halted to prevent damage.");
    draw_string(10, 100, 0xAAAAAA, "Please restart your computer.");
    draw_string(10, 120, 0xAAAAAA, "If this error persists, check your code.");
    
    // Additional debug info
    draw_string(10, 150, 0x888888, "Technical Information:");
    draw_string(10, 170, 0x888888, "- Check VGA text output for details");
    draw_string(10, 190, 0x888888, "- Review exception type above");
    draw_string(10, 210, 0x888888, "- Examine registers in fault_handler");
    
    swap_buffers();
    
    // Halt forever
    for (;;) {
        asm volatile("hlt");
    }  
}
