#include "vmm.h"
#include "pmm.h"

// 64-bit Virtual Memory Manager (stub - boot.asm handles paging)

void vmm_init(void) {
    // Boot.asm sets up 4-level page tables
    // This is a stub for future expansion
}

void vmm_map_page(uint64_t virt, uint64_t phys, uint32_t flags) {
    // Stub - would manipulate page tables
    (void)virt;
    (void)phys;
    (void)flags;
}

void vmm_unmap_page(uint64_t virt) {
    // Stub - would clear page table entry
    (void)virt;
}
