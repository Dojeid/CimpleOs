#include "heap.h"
#include "pmm.h"
#include "string.h"

// Simple heap implementation
// For now, just uses PMM blocks (4KB each)
// A real heap would have smaller granularity

typedef struct heap_block {
    size_t size;
    int is_free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_start = 0;

void heap_init() {
    // Allocate first block from PMM
    heap_start = (heap_block_t*)pmm_alloc_block();
    if (heap_start) {
        heap_start->size = 4096 - sizeof(heap_block_t);
        heap_start->is_free = 1;
        heap_start->next = 0;
    }
}

void* malloc(size_t size) {
    if (!heap_start) heap_init();
    if (!heap_start) return 0; // Out of memory
    
    heap_block_t* current = heap_start;
    
    // Find free block
    while (current) {
        if (current->is_free && current->size >= size) {
            current->is_free = 0;
            return (void*)((char*)current + sizeof(heap_block_t));
        }
        
        // If at end and no block found, allocate new one
        if (!current->next) {
            heap_block_t* new_block = (heap_block_t*)pmm_alloc_block();
            if (!new_block) return 0;
            
            new_block->size = 4096 - sizeof(heap_block_t);
            new_block->is_free = 0;
            new_block->next = 0;
            current->next = new_block;
            
            return (void*)((char*)new_block + sizeof(heap_block_t));
        }
        
        current = current->next;
    }
    
    return 0;
}

void free(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));
    block->is_free = 1;
}
