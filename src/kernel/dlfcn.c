#include "kernel/dlfcn.h"
#include "kernel/elf_loader.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"

typedef struct {
    char path[128];
    void* base_addr;
    size_t size;
} dl_handle_t;

static char last_dlerror[128] = "";

void* dlopen(const char* filename, int flags) {
    if (!filename) return NULL;
    
    vfs_node_t* node = vfs_lookup(filename);
    if (!node || node->type != VFS_FILE) {
        sprintf(last_dlerror, "dlopen: shared object file not found: %s", filename);
        return NULL;
    }
    
    uint64_t entry = 0;
    if (!elf_load_executable(node->data, node->size, &entry)) {
        sprintf(last_dlerror, "dlopen: invalid ELF shared object: %s", filename);
        return NULL;
    }
    
    dl_handle_t* handle = (dl_handle_t*)malloc(sizeof(dl_handle_t));
    if (!handle) return NULL;
    
    strncpy(handle->path, filename, sizeof(handle->path) - 1);
    handle->base_addr = (void*)entry;
    handle->size = node->size;
    
    printf("[DynamicLinker] Loaded shared library '%s' @ %p\n", filename, (void*)entry);
    return handle;
}

void* dlsym(void* handle, const char* symbol) {
    if (!handle || !symbol) return NULL;
    
    dl_handle_t* dl = (dl_handle_t*)handle;
    printf("[DynamicLinker] Resolving symbol '%s' in %s\n", symbol, dl->path);
    return dl->base_addr;
}

int dlclose(void* handle) {
    if (handle) {
        free(handle);
        return 0;
    }
    return -1;
}

char* dlerror(void) {
    if (last_dlerror[0]) {
        return last_dlerror;
    }
    return NULL;
}
