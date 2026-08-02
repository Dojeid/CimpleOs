#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

// Initialize heap
void heap_init();

// Memory allocation
void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void  free(void* ptr);

#define kmalloc(sz) malloc(sz)
#define kfree(ptr) free(ptr)

// Real-time Heap Statistics API
void heap_get_stats(size_t* out_used, size_t* out_free, size_t* out_largest_free);

#endif
