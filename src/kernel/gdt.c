#include "gdt.h"

// 64-bit GDT (simpler than 32-bit - segmentation mostly unused)
struct gdt_entry gdt_entries[5];
struct gdt_ptr gdt_ptr;

extern void gdt_flush(uint64_t);

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;
    
    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

void gdt_init() {
    gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_ptr.base  = (uint64_t)&gdt_entries;
    
    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // 64-bit Code segment
    // Base = 0, Limit = 0xFFFFF (4GB)
    // Access = 0x9A (Present, DPL=0, Executable, Readable)
    // Granularity = 0xAF (64-bit code, 4KB granularity)
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xAF);
    
    // 64-bit Data segment  
    // Access = 0x92 (Present, DPL=0, Writable)
    // Granularity = 0xCF (32-bit operand, 4KB granularity)
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);
    
    // User mode code (ring 3)
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xAF);
    
    // User mode data (ring 3)
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);
    
    gdt_flush((uint64_t)&gdt_ptr);
}
