// ============================================================
// src/lib/new.cpp
// Kernel C++ operator new / delete (placement-new only, no heap)
// ============================================================
// Rules for kernel C++:
//   - No exceptions (-fno-exceptions)
//   - No RTTI (-fno-rtti)
//   - Global operator new MUST be defined or linker errors occur
//   - Use placement new for stack/static allocation
//   - Use kmalloc/free from heap.h for dynamic allocation
// ============================================================

#include <stddef.h>

extern "C" {
    #include "mm/heap.h"
}

// ── Global operator new (calls kmalloc) ──────────────────────
void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

// ── Global operator delete ────────────────────────────────────
void operator delete(void* ptr) noexcept {
    free(ptr);
}

void operator delete[](void* ptr) noexcept {
    free(ptr);
}

// Sized delete (C++14 requirement)
void operator delete(void* ptr, size_t) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    free(ptr);
}

// ── Placement new (zero-overhead, no allocation) ─────────────
// Defined inline in <new> normally — we provide it here.
inline void* operator new(size_t, void* p) noexcept { return p; }
inline void* operator new[](size_t, void* p) noexcept { return p; }
inline void  operator delete(void*, void*) noexcept {}
inline void  operator delete[](void*, void*) noexcept {}
