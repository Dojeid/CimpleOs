#include "pmm.h"

static uint32_t memory_size = 0;
static uint32_t used_blocks = 0;
static uint32_t max_blocks = 0;
static uint32_t* memory_map = 0;

void mmap_set(int bit) {
    memory_map[bit / 32] |= (1 << (bit % 32));
}

void mmap_unset(int bit) {
    memory_map[bit / 32] &= ~(1 << (bit % 32));
}

int mmap_test(int bit) {
    return memory_map[bit / 32] & (1 << (bit % 32));
}

int mmap_first_free() {
    for (uint32_t i = 0; i < max_blocks / 32; i++) {
        if (memory_map[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                int bit = 1 << j;
                if (!(memory_map[i] & bit))
                    return i * 32 + j;
            }
        }
    }
    return -1;
}

void pmm_init_region(uint32_t base, uint32_t size) {
    int align = base / BLOCK_SIZE;
    int blocks = size / BLOCK_SIZE;
    for (; blocks > 0; blocks--) {
        mmap_unset(align++);
        used_blocks--;
    }
    mmap_set(0); // First block always used
}

void pmm_deinit_region(uint32_t base, uint32_t size) {
    int align = base / BLOCK_SIZE;
    int blocks = size / BLOCK_SIZE;
    for (; blocks > 0; blocks--) {
        mmap_set(align++);
        used_blocks++;
    }
}

void pmm_init(struct multiboot_info* mb_info) {
    memory_size = 0;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mb_info->mmap_addr;
    
    // Calculate total memory
    while((uint32_t)mmap < mb_info->mmap_addr + mb_info->mmap_length) {
        if (mmap->type == 1) { // Available memory
            memory_size += mmap->len_low; // Assuming 32-bit for now
        }
        mmap = (multiboot_memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(unsigned int));
    }

    max_blocks = memory_size / BLOCK_SIZE;
    used_blocks = max_blocks;

    // Place bitmap at the end of the kernel (hardcoded for now, ideally use a symbol)
    // Assuming kernel ends around 1MB + size. Let's put it at 4MB mark for safety or after modules.
    // For simplicity in this step, we'll put it at 0x1000000 (16MB) to avoid collisions if we have enough RAM.
    // Better: Put it right after the kernel.
    // Let's assume we have at least 32MB.
    // Place bitmap at 4MB (0x00400000) to ensure it's inside the 16MB identity map
    // (0x1000000 was causing a Page Fault because it was just outside the mapped range)
    memory_map = (uint32_t*)0x00400000; 

    // By default, all memory is "used"
    for(uint32_t i=0; i<max_blocks/32; i++) {
        memory_map[i] = 0xFFFFFFFF;
    }

    // Now mark available regions as free
    mmap = (multiboot_memory_map_t*)mb_info->mmap_addr;
    while((uint32_t)mmap < mb_info->mmap_addr + mb_info->mmap_length) {
        if (mmap->type == 1) {
            pmm_init_region(mmap->addr_low, mmap->len_low);
        }
        mmap = (multiboot_memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(unsigned int));
    }
    
    // Mark kernel and bitmap as used!
    // Kernel (0 - ~1MB+), Bitmap (16MB - ...)
    pmm_deinit_region(0, 0x200000); // Reserve first 2MB for kernel/BIOS/GRUB
    pmm_deinit_region((uint32_t)memory_map, (max_blocks / 8)); // Reserve bitmap itself
}

void* pmm_alloc_block() {
    if (pmm_get_free_memory() <= 0) return 0;
    int frame = mmap_first_free();
    if (frame == -1) return 0;
    mmap_set(frame);
    used_blocks++;
    return (void*)(frame * BLOCK_SIZE);
}

void pmm_free_block(void* p) {
    uint32_t addr = (uint32_t)p;
    int frame = addr / BLOCK_SIZE;
    mmap_unset(frame);
    used_blocks--;
}

uint32_t pmm_get_free_memory() {
    return (max_blocks - used_blocks) * BLOCK_SIZE;
}

uint32_t pmm_get_total_memory() {
    return memory_size;
}
