#include "vmm.h"
#include "pmm.h"
#include "common.h"
#include "lib/io.h"
#include "lib/string.h"
#include "kernel/gdt.h"

// External references to page tables defined in boot.asm
extern uint64_t pml4_table;
extern uint64_t pdpt_table;
extern uint64_t pd_table_0;
extern uint64_t pd_table_1;
extern uint64_t pd_table_2;
extern uint64_t pd_table_3;

// Current active page table (kernel initially)
static vmm_pagetable_t* current_pagetable = NULL;

// Get index into page table level (bits 39-47 for PML4, 30-38 for PDPT, etc.)
#define PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)
#define PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)
#define PD_INDEX(virt)   (((virt) >> 21) & 0x1FF)
#define PT_INDEX(virt)   (((virt) >> 12) & 0x1FF)

// Get page table entry address and flags
#define PTE_ADDR(pte) ((pte) & ~0xFFFULL)
#define PTE_FLAGS(pte) ((pte) & 0xFFFULL)
#define PTE_IS_PRESENT(pte) ((pte) & PTE_FLAG_PRESENT)
#define PTE_IS_HUGE(pte) ((pte) & PTE_FLAG_HUGE)

// Static page table for kernel (identity mapped initially)
static vmm_pagetable_t kernel_pagetable = {
    .pml4 = (uint64_t*)&pml4_table,
    .pdpt = (uint64_t*)&pdpt_table,
    .pd[0] = (uint64_t*)&pd_table_0,
    .pd[1] = (uint64_t*)&pd_table_1,
    .pd[2] = (uint64_t*)&pd_table_2,
    .pd[3] = (uint64_t*)&pd_table_3,
    .physical_base = (uint64_t)&pml4_table
};

// Allocate a new page table page from PMM
static uint64_t* vmm_alloc_pagetable_page(void) {
    void* frame = pmm_alloc_frame();
    if (!frame) return NULL;
    
    // Zero the page
    memset((uint8_t*)frame, 0, PAGE_SIZE);
    return (uint64_t*)frame;
}

// Free a page table page back to PMM
static void vmm_free_pagetable_page(uint64_t* page) {
    if (page) {
        pmm_free_frame((void*)page);
    }
}

// Get page table entry for virtual address (returns NULL if not present)
uint64_t* vmm_get_pte(uint64_t* pml4, uint64_t virt) {
    if (!pml4) return NULL;
    
    uint64_t pml4e = pml4[PML4_INDEX(virt)];
    if (!PTE_IS_PRESENT(pml4e)) return NULL;
    
    uint64_t* pdpt = (uint64_t*)PTE_ADDR(pml4e);
    uint64_t pdpte = pdpt[PDPT_INDEX(virt)];
    if (!PTE_IS_PRESENT(pdpte)) return NULL;
    
    uint64_t* pd = (uint64_t*)PTE_ADDR(pdpte);
    uint64_t pde = pd[PD_INDEX(virt)];
    if (!PTE_IS_PRESENT(pde)) return NULL;
    
    uint64_t* pt = (uint64_t*)PTE_ADDR(pde);
    return &pt[PT_INDEX(virt)];
}

// Get or create page table entry for virtual address
static uint64_t* vmm_get_or_create_pte(uint64_t* pml4, uint64_t virt) {
    if (!pml4) return NULL;
    
    uint64_t* pdpt = NULL;
    uint64_t pdpt_phys = 0;
    uint64_t pdpt_idx = PML4_INDEX(virt);
    bool new_pdpt_alloc = false;
    bool new_pd_alloc = false;
    
    // Get or create PML4 entry
    if (PTE_IS_PRESENT(pml4[pdpt_idx])) {
        pdpt = (uint64_t*)PTE_ADDR(pml4[pdpt_idx]);
    } else {
        pdpt = vmm_alloc_pagetable_page();
        if (!pdpt) return NULL;
        new_pdpt_alloc = true;
        pdpt_phys = (uint64_t)pdpt;
        pml4[pdpt_idx] = pdpt_phys | (PTE_FLAG_PRESENT | PTE_FLAG_WRITE);
    }
    
    uint64_t* pd = NULL;
    uint64_t pd_phys = 0;
    uint64_t pdpt_idx_virt = PDPT_INDEX(virt);
    
    // Get or create PDPT entry
    if (PTE_IS_PRESENT(pdpt[pdpt_idx_virt])) {
        pd = (uint64_t*)PTE_ADDR(pdpt[pdpt_idx_virt]);
    } else {
        pd = vmm_alloc_pagetable_page();
        if (!pd) {
            if (new_pdpt_alloc) {
                vmm_free_pagetable_page(pdpt);
                pml4[pdpt_idx] = 0;
            }
            return NULL;
        }
        new_pd_alloc = true;
        pd_phys = (uint64_t)pd;
        pdpt[pdpt_idx_virt] = pd_phys | (PTE_FLAG_PRESENT | PTE_FLAG_WRITE);
    }
    
    uint64_t* pt = NULL;
    uint64_t pt_phys = 0;
    uint64_t pd_idx_virt = PD_INDEX(virt);
    
    // Get or create PD entry
    if (PTE_IS_PRESENT(pd[pd_idx_virt])) {
        pt = (uint64_t*)PTE_ADDR(pd[pd_idx_virt]);
    } else {
        pt = vmm_alloc_pagetable_page();
        if (!pt) {
            if (new_pd_alloc) {
                vmm_free_pagetable_page(pd);
                pdpt[pdpt_idx_virt] = 0;
            }
            if (new_pdpt_alloc) {
                vmm_free_pagetable_page(pdpt);
                pml4[pdpt_idx] = 0;
            }
            return NULL;
        }
        pt_phys = (uint64_t)pt;
        pd[pd_idx_virt] = pt_phys | (PTE_FLAG_PRESENT | PTE_FLAG_WRITE);
    }
    
    return &pt[PT_INDEX(virt)];
}

// Map a single 4KB page
void vmm_map_page(uint64_t virt, uint64_t phys, uint32_t flags) {
    uint64_t* pml4 = current_pagetable ? current_pagetable->pml4 : NULL;
    if (!pml4) return;
    uint64_t* pte = vmm_get_or_create_pte(pml4, virt);
    if (!pte) return;
    
    // Set the page table entry
    uint64_t pte_value = phys | flags;
    *pte = pte_value;
    
    // Flush TLB entry for this address
    asm volatile("invlpg (%0)" :: "r"(virt));
}

// Unmap a single page
void vmm_unmap_page(uint64_t virt) {
    uint64_t* pml4 = current_pagetable ? current_pagetable->pml4 : NULL;
    if (!pml4) return;
    uint64_t* pte = vmm_get_pte(pml4, virt);
    
    if (pte && PTE_IS_PRESENT(*pte)) {
        // Clear the page table entry
        *pte = 0;
        
        // Flush TLB entry
        asm volatile("invlpg (%0)" :: "r"(virt));
    }
}

// Translate virtual address to physical
uint64_t vmm_translate(uint64_t virt) {
    uint64_t* pml4 = current_pagetable ? current_pagetable->pml4 : NULL;
    if (!pml4) return 0;
    
    uint64_t pml4e = pml4[PML4_INDEX(virt)];
    if (!PTE_IS_PRESENT(pml4e)) return 0;
    
    uint64_t* pdpt = (uint64_t*)PTE_ADDR(pml4e);
    uint64_t pdpte = pdpt[PDPT_INDEX(virt)];
    if (!PTE_IS_PRESENT(pdpte)) return 0;
    
    uint64_t* pd = (uint64_t*)PTE_ADDR(pdpte);
    uint64_t pde = pd[PD_INDEX(virt)];
    if (!PTE_IS_PRESENT(pde)) return 0;
    
    // Check if huge page (2MB)
    if (PTE_IS_HUGE(pde)) {
        return PTE_ADDR(pde) + (virt & ((1ULL << 21) - 1));
    }
    
    uint64_t* pt = (uint64_t*)PTE_ADDR(pde);
    uint64_t pte = pt[PT_INDEX(virt)];
    if (!PTE_IS_PRESENT(pte)) return 0;
    
    return PTE_ADDR(pte) + (virt & (PAGE_SIZE - 1));
}

// Create a new page table
uint64_t vmm_create_pagetable(void) {
    // Allocate a new page table page for PML4
    uint64_t* pml4 = vmm_alloc_pagetable_page();
    if (!pml4) return 0;
    
    uint64_t pml4_phys = (uint64_t)pml4;
    
    // Copy kernel mappings from current page table (entries 256-511 for kernel space)
    if (current_pagetable && current_pagetable->pml4) {
        for (int i = 256; i < 512; i++) {
            if (current_pagetable->pml4[i] & PTE_FLAG_PRESENT) {
                pml4[i] = current_pagetable->pml4[i];
            }
        }
    }
    
    return pml4_phys;
}

// Switch to a new page table
void vmm_switch_pagetable(uint64_t pml4_pa) {
    if (current_pagetable) {
        current_pagetable->physical_base = pml4_pa;
    }
    asm volatile("mov %0, %%cr3" :: "r"(pml4_pa) : "memory");
}

// Get current active page table physical address
uint64_t vmm_get_active_pagetable(void) {
    if (!current_pagetable) return 0;
    return current_pagetable->physical_base;
}

// Clone a page table (for fork())
void vmm_clone_pagetable(uint64_t new_pml4_pa, uint64_t old_pml4_pa) {
    uint64_t* new_pml4 = (uint64_t*)new_pml4_pa;
    uint64_t* old_pml4 = (uint64_t*)old_pml4_pa;
    
    // Copy all PML4 entries
    for (int i = 0; i < 512; i++) {
        new_pml4[i] = old_pml4[i];
    }
    
    // For each present PML4 entry, we need to copy the PDPT
    for (int i = 0; i < 512; i++) {
        if (new_pml4[i] & PTE_FLAG_PRESENT) {
            uint64_t* old_pdpt = (uint64_t*)PTE_ADDR(new_pml4[i]);
            uint64_t* new_pdpt = vmm_alloc_pagetable_page();
            if (!new_pdpt) continue;
            
            // Copy PDPT contents
            for (int j = 0; j < 512; j++) {
                new_pdpt[j] = old_pdpt[j];
            }
            
            // Update PML4 entry to point to new PDPT
            uint64_t pdpt_phys = (uint64_t)new_pdpt;
            uint64_t flags = PTE_FLAGS(new_pml4[i]);
            new_pml4[i] = pdpt_phys | flags;
            
            // For each present PDPT entry, copy the PD
            for (int j = 0; j < 512; j++) {
                if (new_pdpt[j] & PTE_FLAG_PRESENT) {
                    uint64_t* old_pd = (uint64_t*)PTE_ADDR(new_pdpt[j]);
                    uint64_t* new_pd = vmm_alloc_pagetable_page();
                    if (!new_pd) continue;
                    
                    // Copy PD contents
                    for (int k = 0; k < 512; k++) {
                        new_pd[k] = old_pd[k];
                    }
                    
                    // Update PDPT entry to point to new PD
                    uint64_t pd_phys = (uint64_t)new_pd;
                    uint64_t pd_flags = PTE_FLAGS(new_pdpt[j]);
                    new_pdpt[j] = pd_phys | pd_flags;
                    
                    // For each present PD entry, copy the PT
                    for (int k = 0; k < 512; k++) {
                        if (new_pd[k] & PTE_FLAG_PRESENT) {
                            uint64_t* old_pt = (uint64_t*)PTE_ADDR(new_pd[k]);
                            uint64_t* new_pt = vmm_alloc_pagetable_page();
                            if (!new_pt) continue;
                            
                            // Copy PT contents
                            for (int l = 0; l < 512; l++) {
                                new_pt[l] = old_pt[l];
                            }
                            
                            // Update PD entry to point to new PT
                            uint64_t pt_phys = (uint64_t)new_pt;
                            uint64_t pt_flags = PTE_FLAGS(new_pd[k]);
                            new_pd[k] = pt_phys | pt_flags;
                        }
                    }
                }
            }
        }
    }
}

// Map a region of memory
bool vmm_map_region(uint64_t virt, uint64_t phys, uint64_t size, uint32_t flags) {
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t i = 0; i < pages; i++) {
        vmm_map_page(virt + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags);
    }
    
    return true;
}

// Unmap a region of memory
void vmm_unmap_region(uint64_t virt, uint64_t size) {
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t i = 0; i < pages; i++) {
        vmm_unmap_page(virt + i * PAGE_SIZE);
    }
}

// Initialize VMM
void vmm_init(void) {
    // Initialize with kernel page table
    current_pagetable = &kernel_pagetable;
    
    // Load CR3 with kernel page table
    asm volatile("mov %0, %%cr3" :: "r"(kernel_pagetable.physical_base) : "memory");
}
