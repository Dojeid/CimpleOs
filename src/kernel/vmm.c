#include "vmm.h"
#include "pmm.h"

#define PAGE_SIZE 4096

// Page Directory Entry Flags
#define PDE_PRESENT 0x1
#define PDE_RW      0x2
#define PDE_USER    0x4

// Page Table Entry Flags
#define PTE_PRESENT 0x1
#define PTE_RW      0x2
#define PTE_USER    0x4

// Page directory - must be 4KB aligned
uint32_t page_directory[1024] __attribute__((aligned(4096)));

// Pre-allocated page tables for kernel space (first 256MB)
// REVERTED: Back to 64 for stability (was causing VirtualBox crash)
uint32_t kernel_page_tables[64][1024] __attribute__((aligned(4096)));

// High memory page table for framebuffer (maps 3.5GB - 4GB region)
uint32_t high_page_table[1024] __attribute__((aligned(4096)));

void vmm_init() {
    // 1. Clear the page directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = PDE_RW; // Read/Write, but NOT present
    }

    // 2. Identity map first 256MB (kernel, PMM bitmap, low memory)
    // REVERTED: Back to 256MB for stability
    for (int i = 0; i < 64; i++) {
        // Fill the page table with identity mappings
        for (int j = 0; j < 1024; j++) {
            uint32_t phys_addr = (i * 1024 * 4096) + (j * 4096);
            kernel_page_tables[i][j] = phys_addr | PTE_PRESENT | PTE_RW;
        }
        
        // Add page table to directory
        uint32_t pt_phys = (uint32_t)&kernel_page_tables[i];
        page_directory[i] = pt_phys | PDE_PRESENT | PDE_RW;
    }

    // 3. Map high memory region for framebuffer (0xE0000000 - 0xE0400000 = 4MB)
    // This is PDE index 896 (0xE0000000 / 0x400000)
    uint32_t high_pde_index = 0xE0000000 >> 22; // = 896
    
    // Map 4MB of high memory identity
    for (int j = 0; j < 1024; j++) {
        uint32_t phys_addr = 0xE0000000 + (j * 4096);
        high_page_table[j] = phys_addr | PTE_PRESENT | PTE_RW;
    }
    
    uint32_t high_pt_phys = (uint32_t)&high_page_table;
    page_directory[high_pde_index] = high_pt_phys | PDE_PRESENT | PDE_RW;

    // 4. Load page directory into CR3
    uint32_t pd_phys = (uint32_t)&page_directory;
    asm volatile("mov %0, %%cr3" :: "r"(pd_phys));
    
    // 5. Enable paging (set bit 31 of CR0)
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    
    // Success! Paging is now enabled
}

// These functions are no longer needed since we pre-map everything
void vmm_map_page(void* phys, void* virt) {
    // Not needed - everything is pre-mapped
    (void)phys;
    (void)virt;
}

void vmm_identity_map_region(uint32_t phys_addr, uint32_t size) {
    // Not needed - everything is pre-mapped
    (void)phys_addr;
    (void)size;
}
