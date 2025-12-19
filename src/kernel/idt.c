#include "idt.h"
#include "io.h"

// The IDT array itself
struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

// --- EXTERNAL ASSEMBLY HANDLERS ---
extern void isr1();   // Keyboard Handler (IRQ 1)
extern void isr12();  // Mouse Handler    (IRQ 12) <--- THIS WAS MISSING!

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
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master starts at 32
    outb(0xA1, 0x28); // Slave starts at 40
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // --- MASKING INTERRUPTS (The "Guru Meditation" Fix) ---
    // 0 = Enabled, 1 = Disabled.
    // Master PIC: Enable IRQ 1 (Keyboard) and IRQ 2 (Cascade)
    // 1111 1001 = 0xF9
    outb(0x21, 0xF9); 

    // Slave PIC: Enable IRQ 12 (Mouse)
    // 1110 1111 = 0xEF
    outb(0xA1, 0xEF);

    // 2. Install Handlers
    // Keyboard (IRQ 1 -> IDT 33)
    idt_set_gate(33, (uint32_t)isr1, 0x08, 0x8E);
    
    // Mouse (IRQ 12 -> IDT 44)
    idt_set_gate(44, (uint32_t)isr12, 0x08, 0x8E);

    // 3. Load IDT
    asm volatile("lidt (%0)" : : "r" (&idt_ptr));
}