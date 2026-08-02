#include "elf_loader.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"

int elf_validate_header(const void* buf, size_t size) {
    if (!buf || size < sizeof(Elf64_Ehdr)) return 0;
    
    const Elf64_Ehdr* hdr = (const Elf64_Ehdr*)buf;
    if (hdr->e_ident[0] != 0x7F ||
        hdr->e_ident[1] != 'E'  ||
        hdr->e_ident[2] != 'L'  ||
        hdr->e_ident[3] != 'F') {
        return 0;
    }
    
    // Check 64-bit ELF (class 2)
    if (hdr->e_ident[4] != 2) return 0;
    
    return 1;
}

uint64_t elf_load_executable(const void* buf, size_t size, uint64_t* out_entry) {
    if (!elf_validate_header(buf, size)) {
        printf("[ELF] Invalid ELF header magic or format.\n");
        return 0;
    }
    
    const Elf64_Ehdr* hdr = (const Elf64_Ehdr*)buf;
    if (out_entry) *out_entry = hdr->e_entry;
    
    const uint8_t* raw_bytes = (const uint8_t*)buf;
    const Elf64_Phdr* phdr = (const Elf64_Phdr*)(raw_bytes + hdr->e_phoff);
    
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint64_t vaddr = phdr[i].p_vaddr;
            uint64_t filesz = phdr[i].p_filesz;
            uint64_t memsz  = phdr[i].p_memsz;
            uint64_t offset = phdr[i].p_offset;
            
            // Map virtual memory pages for segment
            uint64_t num_pages = (memsz + 4095) / 4096;
            for (uint64_t p = 0; p < num_pages; p++) {
                uint64_t page_vaddr = (vaddr & ~0xFFFUL) + (p * 4096);
                void* phys_page = pmm_alloc_frame();
                vmm_map_page(page_vaddr, (uint64_t)(uintptr_t)phys_page, 0x07); // User + Writable + Present
            }
            
            // Copy file payload into virtual segment memory
            if (filesz > 0 && (offset + filesz <= size)) {
                memcpy((void*)vaddr, raw_bytes + offset, filesz);
            }
            
            // Zero out .bss segment space (memsz > filesz)
            if (memsz > filesz) {
                memset((void*)(vaddr + filesz), 0, memsz - filesz);
            }
            
            printf("[ELF] Loaded PT_LOAD segment 0x%lX (size: %lu bytes)\n", vaddr, memsz);
        }
    }
    
    return hdr->e_entry;
}
