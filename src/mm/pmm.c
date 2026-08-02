#include "pmm.h"
#include "lib/string.h"

#define PAGE_SIZE 4096
#define BITMAP_SIZE 32768
// The static bitmap tracks BITMAP_SIZE * 32 frames (4GB max).
#define MAX_MANAGEABLE ((uint64_t)BITMAP_SIZE * 32 * PAGE_SIZE)

static uint32_t bitmap[BITMAP_SIZE];
static uint64_t total_memory;  // 64-bit
static uint64_t used_frames;   // 64-bit

static void mmap_set(uint64_t bit) {
    if (bit >= (uint64_t)BITMAP_SIZE * 32) return;
    bitmap[bit / 32] |= (1U << (bit % 32));
}

static void mmap_unset(uint64_t bit) {
    if (bit >= (uint64_t)BITMAP_SIZE * 32) return;
    bitmap[bit / 32] &= ~(1U << (bit % 32));
}

static int mmap_test(uint64_t bit) __attribute__((unused));
static int mmap_test(uint64_t bit) {
    if (bit >= (uint64_t)BITMAP_SIZE * 32) return 1;  // Out of range = used
    return (bitmap[bit / 32] & (1U << (bit % 32))) != 0;
}

static int64_t mmap_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1U << j)))
                    return (int64_t)i * 32 + j;
            }
        }
    }
    return -1;
}

void pmm_init(uint64_t mem_size) {
    // Clamp to the range the bitmap can represent (4GB).
    if (mem_size > MAX_MANAGEABLE) mem_size = MAX_MANAGEABLE;
    total_memory = mem_size;
    used_frames = 4096; // Reserve initial 16MB kernel base allocation (4096 frames * 4KB)
    
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0;
    }
    for (uint64_t f = 0; f < 4096; f++) {
        mmap_set(f);
    }
}

#include "drivers/video/vga.h"

void* pmm_alloc_frame(void) {
    int64_t frame = mmap_first_free();
    if (frame == -1) {
        vga_print("[PMM] CRITICAL: Physical memory exhausted! Frame allocation failed.\n");
        return NULL;
    }
    
    mmap_set((uint64_t)frame);
    used_frames++;
    
    uint64_t addr = (uint64_t)frame * PAGE_SIZE;
    return (void*)(uintptr_t)addr;
}

void pmm_free_frame(void* frame) {
    uint64_t addr = (uint64_t)(uintptr_t)frame;
    uint64_t frame_num = addr / PAGE_SIZE;
    mmap_unset(frame_num);
    if (used_frames > 0) used_frames--;
}

uint64_t pmm_get_total_memory(void) {
    return total_memory;
}

uint64_t pmm_get_free_memory(void) {
    return total_memory - (used_frames * PAGE_SIZE);
}
