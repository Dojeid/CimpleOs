#include "idt.h"
#include "io.h"
#include "graphics.h"
#include "panic.h"
#include "printf.h"
#include "string.h"

// The IDT array itself
struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

// --- EXTERNAL ASSEMBLY HANDLERS ---
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void isr32(); // Timer
extern void isr33(); // Keyboard
extern void isr44(); // Mouse

extern void timer_handler();
extern void keyboard_handler();
extern void mouse_handler();

// Set a single entry in the table
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

void init_idt() {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 1. Remap the PIC (Programmable Interrupt Controller)
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00); // Enable all IRQs

    // 2. Install Exception Handlers (0-31)
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    // 3. Install IRQ Handlers
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E); // Timer
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E); // Keyboard
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E); // Mouse

    // 4. Load IDT
    asm volatile("lidt (%0)" : : "r" (&idt_ptr));
}

// Struct for registers passed from ASM
struct registers {
    uint32_t ds, es, fs, gs;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

// Stack trace helper
static void print_stack_trace(uint32_t ebp, uint32_t eip) {
    draw_string(10, 240, 0xFFFFFF, "Stack Trace:");
    char buf[128];
    
    sprintf(buf, "  [0] EIP=0x%08X", eip);
    draw_string(10, 260, 0xAAFFFF, buf);
    
    // Walk the stack (max 8 frames)
    for (int frame = 1; frame < 8 && ebp != 0; frame++) {
        uint32_t* stack = (uint32_t*)ebp;
        uint32_t ret_addr = stack[1];  // Return address
        ebp = stack[0];  // Previous EBP
        
        if (ret_addr < 0x100000 || ret_addr > 0x10000000) break;
        
        sprintf(buf, "  [%d] EIP=0x%08X", frame, ret_addr);
        draw_string(10, 260 + frame * 20, 0xAAFFFF, buf);
    }
}

// Memory dump around fault
static void print_memory_dump(uint32_t addr) {
    draw_string(10, 450, 0xFFFFFF, "Memory near fault:");
    char buf[128];
    
    // Dump 4 lines of 16 bytes
    for (int line = 0; line < 4; line++) {
        uint32_t line_addr = (addr & 0xFFFFFFF0) + (line * 16);
        sprintf(buf, "0x%08X:", line_addr);
        
        // Try to read memory (might fault again!)
        for (int i = 0; i < 16; i++) {
            char hex[4];
            sprintf(hex, " %02X", ((uint8_t*)line_addr)[i]);
            int len = strlen(buf);
            strcpy(buf + len, hex);
        }
        
        draw_string(10, 470 + line * 20, 0xCCCCCC, buf);
    }
}

void fault_handler(struct registers *r) {
    if (r->int_no < 32) {
        // Exception! Show DETAILED diagnostic information
        clear_screen(0x990000);
        
        // Header
        char buf[256];
        sprintf(buf, "=== KERNEL PANIC: EXCEPTION #%d ===", r->int_no);
        draw_string(10, 10, 0xFFFFFF, buf);
        
        sprintf(buf, "%s", exception_messages[r->int_no]);
        draw_string(10, 30, 0xFFFF00, buf);
        
        // Error code with explanation
        sprintf(buf, "Error Code: 0x%08X", r->err_code);
        draw_string(10, 60, 0xFFAAAA, buf);
        
        if (r->int_no == 14 && r->err_code != 0) {
            // Page fault error code breakdown
            sprintf(buf, "  [%s] [%s] [%s]",
                    (r->err_code & 0x1) ? "Protection" : "Not Present",
                    (r->err_code & 0x2) ? "Write" : "Read",
                    (r->err_code & 0x4) ? "User" : "Kernel");
            draw_string(10, 80, 0xCCCCCC, buf);
        }
        
        // Register dump
        draw_string(10, 110, 0xFFFFFF, "Registers:");
        sprintf(buf, "EAX=0x%08X  EBX=0x%08X  ECX=0x%08X  EDX=0x%08X", 
                r->eax, r->ebx, r->ecx, r->edx);
        draw_string(10, 130, 0xAAFFAA, buf);
        
        sprintf(buf, "ESI=0x%08X  EDI=0x%08X  EBP=0x%08X  ESP=0x%08X",
                r->esi, r->edi, r->ebp, r->esp);
        draw_string(10, 150, 0xAAFFAA, buf);
        
        sprintf(buf, "EIP=0x%08X  EFLAGS=0x%08X  CS=0x%04X  DS=0x%04X",
                r->eip, r->eflags, r->cs, r->ds);
        draw_string(10, 170, 0xAAFFAA, buf);
        
        // EFLAGS breakdown
        sprintf(buf, "Flags: [%s%s%s%s%s%s]",
                (r->eflags & (1<<0)) ? "CF " : "",
                (r->eflags & (1<<6)) ? "ZF " : "",
                (r->eflags & (1<<7)) ? "SF " : "",
                (r->eflags & (1<<9)) ? "IF " : "",
                (r->eflags & (1<<10)) ? "DF " : "",
                (r->eflags & (1<<11)) ? "OF " : "");
        draw_string(10, 190, 0x888888, buf);
        
        // Stack trace
        print_stack_trace(r->ebp, r->eip);
        
        // Specific diagnostics
        if (r->int_no == 14) {
            uint32_t cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            sprintf(buf, "Page Fault Address: 0x%08X", cr2);
            draw_string(10, 400, 0xFFFFFF, buf);
            
            sprintf(buf, "Attempted to %s %s memory at 0x%08X",
                    (r->err_code & 0x2) ? "write" : "read",
                    (r->err_code & 0x1) ? "protected" : "non-present",
                    cr2);
            draw_string(10, 420, 0xFFAAAA, buf);
        }
        
        swap_buffers();
        
        // VGA text output
        printf("\n\n*** KERNEL PANIC ***\n");
        printf("Exception #%d: %s\n", r->int_no, exception_messages[r->int_no]);
        printf("EIP: 0x%08X  Error: 0x%X\n", r->eip, r->err_code);
        printf("EAX: 0x%08X  EBX: 0x%08X  ECX: 0x%08X  EDX: 0x%08X\n",
               r->eax, r->ebx, r->ecx, r->edx);
        printf("System halted. Press Ctrl+Alt+Del to restart.\n");
        
        // Halt forever
        for(;;) asm("hlt");
        
    } else if (r->int_no == 32) {
        timer_handler();
    } else if (r->int_no == 33) {
        keyboard_handler();
        outb(0x20, 0x20);
    } else if (r->int_no == 44) {
        mouse_handler();
        outb(0x20, 0x20);
        outb(0xA0, 0x20);
    }
}