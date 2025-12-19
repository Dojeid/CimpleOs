#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"

#define BLOCK_SIZE 4096
#define BLOCKS_PER_BYTE 8

void pmm_init(struct multiboot_info* mb_info);
void* pmm_alloc_block();
void pmm_free_block(void* p);
uint32_t pmm_get_free_memory();
uint32_t pmm_get_total_memory();

#endif
