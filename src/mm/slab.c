#include "mm/slab.h"
#include "mm/heap.h"
#include "lib/string.h"

#define SLAB_32   32
#define SLAB_64   64
#define SLAB_128  128
#define SLAB_256  256
#define SLAB_512  512

typedef struct slab_obj {
    struct slab_obj* next;
} slab_obj_t;

typedef struct {
    size_t obj_size;
    slab_obj_t* free_list;
    uint32_t total_allocs;
} slab_cache_t;

static slab_cache_t cache_32;
static slab_cache_t cache_64;
static slab_cache_t cache_128;
static slab_cache_t cache_256;
static slab_cache_t cache_512;

void slab_init(void) {
    cache_32.obj_size = SLAB_32; cache_32.free_list = NULL; cache_32.total_allocs = 0;
    cache_64.obj_size = SLAB_64; cache_64.free_list = NULL; cache_64.total_allocs = 0;
    cache_128.obj_size = SLAB_128; cache_128.free_list = NULL; cache_128.total_allocs = 0;
    cache_256.obj_size = SLAB_256; cache_256.free_list = NULL; cache_256.total_allocs = 0;
    cache_512.obj_size = SLAB_512; cache_512.free_list = NULL; cache_512.total_allocs = 0;
}

static void* slab_alloc_from_cache(slab_cache_t* cache) {
    if (cache->free_list) {
        slab_obj_t* obj = cache->free_list;
        cache->free_list = obj->next;
        cache->total_allocs++;
        return (void*)obj;
    }
    void* ptr = malloc(cache->obj_size);
    if (ptr) cache->total_allocs++;
    return ptr;
}

static void slab_free_to_cache(slab_cache_t* cache, void* ptr) {
    if (!ptr) return;
    slab_obj_t* obj = (slab_obj_t*)ptr;
    obj->next = cache->free_list;
    cache->free_list = obj;
}

void* kmem_cache_alloc(size_t size) {
    if (size <= 32)  return slab_alloc_from_cache(&cache_32);
    if (size <= 64)  return slab_alloc_from_cache(&cache_64);
    if (size <= 128) return slab_alloc_from_cache(&cache_128);
    if (size <= 256) return slab_alloc_from_cache(&cache_256);
    if (size <= 512) return slab_alloc_from_cache(&cache_512);
    return malloc(size);
}

void kmem_cache_free(void* ptr, size_t size) {
    if (!ptr) return;
    if (size <= 32)  { slab_free_to_cache(&cache_32, ptr); return; }
    if (size <= 64)  { slab_free_to_cache(&cache_64, ptr); return; }
    if (size <= 128) { slab_free_to_cache(&cache_128, ptr); return; }
    if (size <= 256) { slab_free_to_cache(&cache_256, ptr); return; }
    if (size <= 512) { slab_free_to_cache(&cache_512, ptr); return; }
    free(ptr);
}
