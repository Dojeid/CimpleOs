#include "gdt.h"

// GDT with 3 entries: null, code, data
struct gdt_entry gdt[3];
struct gdt_ptr gp;

// External assembly function to load GDT
extern void gdt_flush(uint32_t);

// Set a GDT gate/entry
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Base address
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    // Limits
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    // Granularity and access flags
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

// Install the GDT
void gdt_install(void) {
    // Set up the GDT pointer
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uint32_t)&gdt;

    // NULL descriptor (required)
    gdt_set_gate(0, 0, 0, 0, 0);

    // Code segment: base=0, limit=0xFFFFFFFF, access=0x9A, granularity=0xCF
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // Data segment: base=0, limit=0xFFFFFFFF, access=0x92, granularity=0xCF
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // Load the GDT
    gdt_flush((uint32_t)&gp);
}
