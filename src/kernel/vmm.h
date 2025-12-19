#ifndef VMM_H
#define VMM_H

#include <stdint.h>

void vmm_init();
void vmm_map_page(void* phys, void* virt);

#endif
