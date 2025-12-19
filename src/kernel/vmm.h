#ifndef VMM_H
#define VMM_H

#include <stdint.h>

void vmm_init();
void vmm_map_page(void* phys, void* virt);
void vmm_identity_map_region(uint32_t phys_addr, uint32_t size);

#endif
