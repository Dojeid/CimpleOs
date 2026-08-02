#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>

// Page table entry flags
#define PTE_FLAG_PRESENT      (1ULL << 0)
#define PTE_FLAG_WRITE        (1ULL << 1)
#define PTE_FLAG_USER         (1ULL << 2)
#define PTE_FLAG_WRITE_THROUGH (1ULL << 3)
#define PTE_FLAG_CACHE_DISABLE (1ULL << 4)
#define PTE_FLAG_ACCESSED     (1ULL << 5)
#define PTE_FLAG_DIRTY        (1ULL << 6)
#define PTE_FLAG_HUGE         (1ULL << 7)  // 2MB/1GB page (PSE)
#define PTE_FLAG_GLOBAL       (1ULL << 8)  // Global page (CR4.PGE)
#define PTE_FLAG_AVAILABLE    (1ULL << 9)  // Available for software
#define PTE_FLAG_NOEXEC       (1ULL << 63) // NX bit (EFER.NXE)

// Flag constants for vmm_map_page
#define VMM_FLAG_PRESENT      PTE_FLAG_PRESENT
#define VMM_FLAG_READWRITE    PTE_FLAG_WRITE
#define VMM_FLAG_USER         PTE_FLAG_USER
#define VMM_FLAG_NX           PTE_FLAG_NOEXEC

// VMM page table structure
typedef struct {
    uint64_t* pml4;
    uint64_t* pdpt;
    uint64_t* pd[4];
    uint64_t physical_base;
} vmm_pagetable_t;

// Initialize VMM
void vmm_init(void);

// Map a virtual page to a physical frame
void vmm_map_page(uint64_t virt, uint64_t phys, uint32_t flags);

// Unmap a virtual page
void vmm_unmap_page(uint64_t virt);

// Get physical address from virtual address (returns 0 if not mapped)
uint64_t vmm_translate(uint64_t virt);

// Create a new page table (for new process)
uint64_t vmm_create_pagetable(void);

// Switch to a new page table (context switch)
void vmm_switch_pagetable(uint64_t pml4_pa);

// Get current active page table physical address
uint64_t vmm_get_active_pagetable(void);

// Map a region of memory
bool vmm_map_region(uint64_t virt, uint64_t phys, uint64_t size, uint32_t flags);

// Unmap a region of memory
void vmm_unmap_region(uint64_t virt, uint64_t size);

// Clone a page table (for fork())
void vmm_clone_pagetable(uint64_t new_pml4_pa, uint64_t old_pml4_pa);

// Get a pointer to the PTE for a virtual address (for advanced use)
uint64_t* vmm_get_pte(uint64_t* pml4, uint64_t virt);

#endif
