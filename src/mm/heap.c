#include "heap.h"
#include "mm/pmm.h"

#define HEAP_START 0x1000000
#define HEAP_SIZE 0x1000000

static uintptr_t heap_start = HEAP_START;
static uintptr_t heap_end = HEAP_START + HEAP_SIZE;
static uintptr_t current_break = HEAP_START;

void heap_init(void) {
    current_break = heap_start;
}

void* malloc(size_t size) {
    if (current_break + size > heap_end) {
        return NULL;
    }
    
    void* ptr = (void*)current_break;
    current_break += size;
    
    // 16-byte alignment for 64-bit
    current_break = (current_break + 15) & ~((uintptr_t)15);
    
    return ptr;
}

void free(void* ptr) {
    // Simple heap - no real free for now
    (void)ptr;
}
