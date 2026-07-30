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

// Detailed 64-bit panic screen
void kernel_panic(const char* message, uint64_t error_code) {
    asm volatile("cli");
    
    // Capture 64-bit CPU registers
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    asm volatile("mov %%rax, %0" : "=r"(rax));
    asm volatile("mov %%rbx, %0" : "=r"(rbx));
    asm volatile("mov %%rcx, %0" : "=r"(rcx));
    asm volatile("mov %%rdx, %0" : "=r"(rdx));
    asm volatile("mov %%rsi, %0" : "=r"(rsi));
    asm volatile("mov %%rdi, %0" : "=r"(rdi));
    asm volatile("mov %%rbp, %0" : "=r"(rbp));
    asm volatile("mov %%rsp, %0" : "=r"(rsp));

    // Red Screen of Death
    clear_screen(0x800000);
    
    // Title
    draw_string(10, 10, 0xFFFFFF, "=== FALKON-OS 64-BIT KERNEL PANIC ===");
    
    char buf[128];
    sprintf(buf, "Fatal Exception: %s", message);
    draw_string(10, 32, 0xFFFF00, buf);
    
    if (error_code != 0) {
        sprintf(buf, "Error Code: 0x%lX", error_code);
        draw_string(10, 52, 0xFFAAAA, buf);
    }
    
    // 64-bit Register Dump
    draw_string(10, 80, 0x38BDF8, "--- 64-Bit Register Dump ---");
    
    sprintf(buf, "RAX: 0x%lX  RBX: 0x%lX  RCX: 0x%lX", rax, rbx, rcx);
    draw_string(10, 100, 0xE2E8F0, buf);
    
    sprintf(buf, "RDX: 0x%lX  RSI: 0x%lX  RDI: 0x%lX", rdx, rsi, rdi);
    draw_string(10, 120, 0xE2E8F0, buf);
    
    sprintf(buf, "RBP: 0x%lX  RSP: 0x%lX", rbp, rsp);
    draw_string(10, 140, 0xE2E8F0, buf);

    // Halt Instructions
    draw_string(10, 175, 0xAAAAAA, "System halted to prevent memory corruption.");
    draw_string(10, 195, 0xAAAAAA, "Please reboot your virtual machine.");
    
    swap_buffers();
    
    for (;;) {
        asm volatile("hlt");
    }  
}

void kpanic(const char* message) {
    kernel_panic(message, 0);
}
