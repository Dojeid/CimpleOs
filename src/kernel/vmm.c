#include "vmm.h"
#include "pmm.h"

#define PAGE_SIZE 4096
#define PT_ENTRIES 1024

// Page Directory Entry Flags
#define PDE_PRESENT 0x1
#define PDE_RW      0x2
#define PDE_USER    0x4

// Page Table Entry Flags
#define PTE_PRESENT 0x1
#define PTE_RW      0x2
#define PTE_USER    0x4

uint32_t* page_directory = 0;

extern void load_page_directory(uint32_t*);
extern void enable_paging();

void vmm_init() {
    // 1. Allocate a Page Directory
    page_directory = (uint32_t*)pmm_alloc_block();
    
    // 2. Clear it (mark all as not present)
    for(int i=0; i<1024; i++) {
        page_directory[i] = 0x00000002; // RW, not present
    }

    // 3. Identity Map the first 16MB (Kernel + PMM Bitmap + Video)
    // We need 4 Page Tables (4 * 4MB = 16MB)
    for(int i=0; i<4; i++) {
        uint32_t* page_table = (uint32_t*)pmm_alloc_block();
        
        // Fill the page table
        for(int j=0; j<1024; j++) {
            uint32_t phys_addr = (i * 1024 * 4096) + (j * 4096);
            page_table[j] = phys_addr | PTE_PRESENT | PTE_RW;
        }

        // Add to directory
        page_directory[i] = ((uint32_t)page_table) | PDE_PRESENT | PDE_RW;
    }

    // 4. Load Page Directory
    // We need a small assembly helper for this, but for now I'll use inline asm
    asm volatile("mov %0, %%cr3":: "r"(page_directory));
    
    // 5. Enable Paging
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable Paging bit
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void vmm_map_page(void* phys, void* virt) {
    // TODO: Implement dynamic mapping
    // This requires traversing the PD, finding/allocating PT, and setting the entry.
}
