#include "pmm.h"
#include "string.h"

#define PAGE_SIZE 4096
#define BITMAP_SIZE 32768

static uint32_t bitmap[BITMAP_SIZE];
static uint64_t total_memory;  // 64-bit
static uint64_t used_frames;   // 64-bit

static void mmap_set(int bit) {
    bitmap[bit / 32] |= (1 << (bit % 32));
}

static void mmap_unset(int bit) {
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

static int mmap_test(int bit) {
    return bitmap[bit / 32] & (1 << (bit % 32));
}

static int mmap_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1 << j)))
                    return i * 32 + j;
            }
        }
    }
    return -1;
}

void pmm_init(uint64_t mem_size) {
    total_memory = mem_size;
    used_frames = 0;
    
    for (int i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0;
    }
}

void* pmm_alloc_frame(void) {
    int frame = mmap_first_free();
    if (frame == -1) return NULL;
    
    mmap_set(frame);
    used_frames++;
    
    uint64_t addr = (uint64_t)frame * PAGE_SIZE;
    return (void*)addr;
}

void pmm_free_frame(void* frame) {
    uint64_t addr = (uint64_t)frame;
    int frame_num = addr / PAGE_SIZE;
    mmap_unset(frame_num);
    used_frames--;
}

uint64_t pmm_get_total_memory(void) {
    return total_memory;
}

uint64_t pmm_get_free_memory(void) {
    return total_memory - (used_frames * PAGE_SIZE);
}
