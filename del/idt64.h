#ifndef IDT64_H
#define IDT64_H

#include <stdint.h>

// 64-bit IDT entry (16 bytes)
struct idt_entry_64 {
    uint16_t base_low;      // Lower 16 bits of handler address
    uint16_t selector;      // Kernel segment selector
    uint8_t  ist;           // Interrupt Stack Table offset (0 = don't switch)
    uint8_t  flags;         // Type and attributes
    uint16_t base_mid;      // Middle 16 bits of handler address
    uint32_t base_high;     // Upper 32 bits of handler address
    uint32_t reserved;      // Always zero
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr_64 {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Initialize IDT
void idt64_init(void);

// Set IDT gate
void idt64_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags);

#endif
