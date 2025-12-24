#ifndef VMM_H
#define VMM_H

#include <stdint.h>

void vmm_init(void);
void vmm_map_page(uint64_t virt, uint64_t phys, uint32_t flags);  // 64-bit addresses
void vmm_unmap_page(uint64_t virt);  // 64-bit address

#endif
