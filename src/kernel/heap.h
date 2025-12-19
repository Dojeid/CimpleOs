#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

// Initialize heap
void heap_init();

// Allocate memory
void* malloc(size_t size);

// Free memory
void free(void* ptr);

#endif
