#include "heap.h"
#include "mm/pmm.h"

#define HEAP_START 0x1000000
#define HEAP_SIZE 0x4000000 // 64 MB Kernel Heap

// Block header: 32 bytes (two pointers + size + magic/used flags).
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

static uint64_t irq_save(void) {
    uint64_t flags;
    asm volatile("pushfq; pop %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static void irq_restore(uint64_t flags) {
    if (flags & 0x200) {
        asm volatile("sti" ::: "memory");
    }
}

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
    uint64_t flags = irq_save();
    if (!head) heap_init();

    size_t need = ALIGN16(size);
    if (need < 16) need = 16;

    for (block_t* b = head; b; b = b->next) {
        if (b->used || b->size < need) continue;

        // Split when leftover can host a new block
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
        irq_restore(flags);
        return (void*)((char*)b + HEADER_SIZE);
    }
    irq_restore(flags);
    return NULL;
}

void* calloc(size_t num, size_t size) {
    if (size != 0 && num > 0xFFFFFFFFu / size) return NULL;
    size_t total = num * size;
    void* ptr = malloc(total);
    if (ptr) {
        extern void* memset(void* dest, int val, size_t count);
        memset(ptr, 0, total);
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }
    
    block_t* b = (block_t*)((char*)ptr - HEADER_SIZE);
    if (b->magic != BLOCK_MAGIC) return NULL;
    
    if (b->size >= ALIGN16(size)) return ptr;
    
    void* new_ptr = malloc(size);
    if (new_ptr) {
        extern void* memcpy(void* dest, const void* src, size_t n);
        memcpy(new_ptr, ptr, b->size);
        free(ptr);
    }
    return new_ptr;
}

void free(void* ptr) {
    if (!ptr) return;
    if (!head) return;

    uint64_t flags = irq_save();
    block_t* b = (block_t*)((char*)ptr - HEADER_SIZE);
    if (b->magic != BLOCK_MAGIC) {
        irq_restore(flags);
        return;
    }

    b->used = 0;

    // Coalesce next free block
    while (b->next && !b->next->used &&
           (char*)b + HEADER_SIZE + b->size == (char*)b->next) {
        b->size += HEADER_SIZE + b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }

    // Coalesce previous free block
    if (b->prev && !b->prev->used &&
        (char*)b->prev + HEADER_SIZE + b->prev->size == (char*)b) {
        b->prev->size += HEADER_SIZE + b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
    irq_restore(flags);
}

void heap_get_stats(size_t* out_used, size_t* out_free, size_t* out_largest_free) {
    uint64_t flags = irq_save();
    size_t used_bytes = 0;
    size_t free_bytes = 0;
    size_t max_free = 0;
    if (head) {
        for (block_t* b = head; b; b = b->next) {
            if (b->magic != BLOCK_MAGIC) break;
            if (b->used) {
                used_bytes += HEADER_SIZE + b->size;
            } else {
                free_bytes += HEADER_SIZE + b->size;
                if (b->size > max_free) max_free = b->size;
            }
        }
    }
    if (out_used) *out_used = used_bytes;
    if (out_free) *out_free = free_bytes;
    if (out_largest_free) *out_largest_free = max_free;
    irq_restore(flags);
}
