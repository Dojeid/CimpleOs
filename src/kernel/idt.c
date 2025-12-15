#include "idt.h"

// Declare an IDT of 256 entries. Although we will only use the first 32.
// The rest exists as a bit of a trap. If any undefined IDT entry is hit, it normally
// causes an "Unhandled Interrupt" exception. Any descriptor for which the 'presence'
// bit is cleared (0) will generate an "Unhandled Interrupt" exception.
struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

// Exists in assembly (we will write this next)
extern void isr1(); // Keyboard interrupt is usually IRQ 1

// Helper to set an entry in the IDT
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // We must uncomment the OR below when we get to using user-mode.
    // It sets the interrupt gate's privilege level to 3.
    idt_entries[num].flags   = flags /* | 0x60 */;
}

void init_idt() {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 0 is null to clear everything out
    // memset(&idt_entries, 0, sizeof(struct idt_entry_struct) * 256);

    // Remap the PIC (Programmable Interrupt Controller)
    // We need to tell the hardware to send keyboard interrupts to us!
    // For now, we are skipping the complex PIC remapping code to keep it simple.
    // We assume the standard boot state where IRQ1 (Keyboard) maps to Interrupt 33 (0x21).
    
    // Set gate 33 (IRQ 1) to point to our 'isr1' function
    // 0x08 is our Code Segment (from your GDT)
    // 0x8E means "Present, Ring 0, Interrupt Gate"
    idt_set_gate(33, (uint32_t)isr1, 0x08, 0x8E);

    // Load the IDT pointer
    asm volatile("lidt (%0)" : : "r" (&idt_ptr));
}