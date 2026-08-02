#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>
#include <stdint.h>

void slab_init(void);
void* kmem_cache_alloc(size_t size);
void kmem_cache_free(void* ptr, size_t size);

#endif // SLAB_H
