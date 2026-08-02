#include "kernel/elf.h"
#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

int elf_validate(const uint8_t* buffer, size_t size) {
    if (!buffer || size < sizeof(Elf32_Ehdr)) return 0;
    
    // Check Magic \x7fELF
    if (buffer[0] != 0x7F || buffer[1] != 'E' || buffer[2] != 'L' || buffer[3] != 'F') {
        return 0;
    }
    
    // Check Class (32-bit or 64-bit)
    if (buffer[EI_CLASS] != ELFCLASS32 && buffer[EI_CLASS] != ELFCLASS64) {
        return 0;
    }
    
    return 1;
}

int elf_load_and_run(const char* filepath, char* const argv[], char* const envp[]) {
    (void)argv;
    (void)envp;
    
    if (!filepath) return -1;
    
    file_t* file = vfs_open(filepath, 0);
    if (!file) {
        vga_print("[ELF Loader] Error: Unable to open binary file ");
        vga_print(filepath);
        vga_print("\n");
        return -1;
    }
    
    // Read file payload
    uint32_t file_size = (file && file->f_dentry && file->f_dentry->d_inode) ? file->f_dentry->d_inode->i_size : 4096;
    if (file_size == 0) file_size = 4096;

    uint8_t* buffer = (uint8_t*)kmalloc(file_size);
    if (!buffer) {
        vfs_close(file);
        return -1;
    }

    int bytes_read = vfs_read(file, file_size, buffer);
    vfs_close(file);

    if (bytes_read <= 0 || !elf_validate(buffer, bytes_read)) {
        vga_print("[ELF Loader] Error: Invalid ELF magic header in ");
        vga_print(filepath);
        vga_print("\n");
        kfree(buffer);
        return -1;
    }

    uint64_t entry_point = 0;

    if (buffer[EI_CLASS] == ELFCLASS64) {
        Elf64_Ehdr* ehdr = (Elf64_Ehdr*)buffer;
        entry_point = ehdr->e_entry;

        vga_print("[ELF Loader] Valid 64-Bit ELF Executable Detected. Entry: 0x");
        char hex[32];
        sprintf(hex, "%x", (uint32_t)entry_point);
        vga_print(hex);
        vga_print("\n");

        Elf64_Phdr* phdr = (Elf64_Phdr*)(buffer + ehdr->e_phoff);
        for (int i = 0; i < ehdr->e_phnum; i++) {
            if (phdr[i].p_type == PT_LOAD) {
                void* segment_mem = kmalloc((size_t)phdr[i].p_memsz);
                if (segment_mem) {
                    memset(segment_mem, 0, (size_t)phdr[i].p_memsz);
                    memcpy(segment_mem, buffer + phdr[i].p_offset, (size_t)phdr[i].p_filesz);
                }
            }
        }
    } else {
        Elf32_Ehdr* ehdr = (Elf32_Ehdr*)buffer;
        entry_point = (uint64_t)ehdr->e_entry;

        vga_print("[ELF Loader] Valid 32-Bit ELF Executable Detected. Entry: 0x");
        char hex[32];
        sprintf(hex, "%x", (uint32_t)entry_point);
        vga_print(hex);
        vga_print("\n");

        Elf32_Phdr* phdr = (Elf32_Phdr*)(buffer + ehdr->e_phoff);
        for (int i = 0; i < ehdr->e_phnum; i++) {
            if (phdr[i].p_type == PT_LOAD) {
                void* segment_mem = kmalloc((size_t)phdr[i].p_memsz);
                if (segment_mem) {
                    memset(segment_mem, 0, (size_t)phdr[i].p_memsz);
                    memcpy(segment_mem, buffer + phdr[i].p_offset, (size_t)phdr[i].p_filesz);
                }
            }
        }
    }

    kfree(buffer);

    // Spawn process for ELF binary
    process_t* proc = process_create(filepath, (void (*)(void))entry_point);
    if (!proc) {
        vga_print("[ELF Loader] Error spawning ELF process.\n");
        return -1;
    }

    vga_print("[ELF Loader] Process spawned successfully (PID: ");
    char pid_str[16];
    sprintf(pid_str, "%u", proc->pid);
    vga_print(pid_str);
    vga_print(").\n");

    return (int)proc->pid;
}
