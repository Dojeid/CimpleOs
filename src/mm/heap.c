#include "heap.h"
#include "mm/pmm.h"

#define HEAP_START 0x1000000
#define HEAP_SIZE 0x1000000

// Block header: 32 bytes (two pointers + size + magic/used flags).
// 16-byte aligned so payloads are always 16-byte aligned for 64-bit.
#define HEADER_SIZE 32
#define BLOCK_MAGIC 0xF41C0DE5u

#define ALIGN16(x) (((x) + 15) & ~((uintptr_t)15))

typedef struct block {
    struct block* next;
    struct block* prev;
    size_t size;          // usable payload size (multiple of 16)
    uint32_t magic;
    uint32_t used;
} block_t;

static uintptr_t heap_start = HEAP_START;
static uintptr_t heap_end = HEAP_START + HEAP_SIZE;
static block_t* head = NULL;

void heap_init(void) {
    heap_start = HEAP_START;
    heap_end = HEAP_START + HEAP_SIZE;
    head = (block_t*)heap_start;
    head->next = NULL;
    head->prev = NULL;
    head->size = heap_end - heap_start - HEADER_SIZE;
    head->magic = BLOCK_MAGIC;
    head->used = 0;
}

void* malloc(size_t size) {
    if (!head) heap_init();

    size_t need = ALIGN16(size);
    if (need < 16) need = 16;

    for (block_t* b = head; b; b = b->next) {
        if (b->used || b->size < need) continue;

        // Split when the leftover can host a new block
        if (b->size >= need + HEADER_SIZE + 16) {
            block_t* nb = (block_t*)((char*)b + HEADER_SIZE + need);
            nb->next = b->next;
            nb->prev = b;
            nb->size = b->size - need - HEADER_SIZE;
            nb->magic = BLOCK_MAGIC;
            nb->used = 0;
            if (b->next) b->next->prev = nb;
            b->next = nb;
            b->size = need;
        }
        b->used = 1;
        return (void*)((char*)b + HEADER_SIZE);
    }
    return NULL;
}

void free(void* ptr) {
    if (!ptr) return;
    if (!head) return;

    block_t* b = (block_t*)((char*)ptr - HEADER_SIZE);
    if (b->magic != BLOCK_MAGIC) return;

    b->used = 0;

    // BUG FIX #4: Coalesce with next free block first
    // We MUST check b->next exists BEFORE accessing it
    while (b->next && !b->next->used &&
           (char*)b + HEADER_SIZE + b->size == (char*)b->next) {
        b->size += HEADER_SIZE + b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }

    // BUG FIX #4: Coalesce with previous free block
    // We MUST check b->prev exists BEFORE accessing it
    if (b->prev && !b->prev->used &&
        (char*)b->prev + HEADER_SIZE + b->prev->size == (char*)b) {
        b->prev->size += HEADER_SIZE + b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}
