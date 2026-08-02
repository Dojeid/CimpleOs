#include "kernel/pe.h"
#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

int pe_validate(const uint8_t* buffer, size_t size) {
    if (!buffer || size < sizeof(IMAGE_DOS_HEADER)) return 0;
    
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)buffer;
    if (dos->e_magic != DOS_MAGIC) return 0;
    
    if (dos->e_lfanew + 4 > size) return 0;
    
    uint32_t pe_sig = *(uint32_t*)(buffer + dos->e_lfanew);
    return pe_sig == PE_MAGIC;
}

int pe_load_and_run(const char* filepath, char* const argv[], char* const envp[]) {
    (void)argv;
    (void)envp;
    
    if (!filepath) return -1;
    
    file_t* file = vfs_open(filepath, 0);
    if (!file) {
        vga_print("[PE Loader] Error: Unable to open Windows PE file ");
        vga_print(filepath);
        vga_print("\n");
        return -1;
    }
    
    uint32_t file_size = (file && file->f_dentry && file->f_dentry->d_inode) ? file->f_dentry->d_inode->i_size : 4096;
    if (file_size == 0) file_size = 4096;

    uint8_t* buffer = (uint8_t*)kmalloc(file_size);
    if (!buffer) {
        vfs_close(file);
        return -1;
    }

    int bytes_read = vfs_read(file, file_size, buffer);
    vfs_close(file);

    if (bytes_read <= 0 || !pe_validate(buffer, bytes_read)) {
        vga_print("[PE Loader] Error: Invalid PE/COFF header in ");
        vga_print(filepath);
        vga_print("\n");
        kfree(buffer);
        return -1;
    }

    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)buffer;
    IMAGE_FILE_HEADER* file_hdr = (IMAGE_FILE_HEADER*)(buffer + dos->e_lfanew + 4);
    IMAGE_OPTIONAL_HEADER64* opt_hdr = (IMAGE_OPTIONAL_HEADER64*)((uint8_t*)file_hdr + sizeof(IMAGE_FILE_HEADER));

    uint64_t entry_point = (uint64_t)opt_hdr->ImageBase + opt_hdr->AddressOfEntryPoint;

    vga_print("[PE Loader] Valid Windows PE Executable Loaded. Entry: 0x");
    char hex[32];
    sprintf(hex, "%x", (uint32_t)entry_point);
    vga_print(hex);
    vga_print("\n");

    IMAGE_SECTION_HEADER* sec = (IMAGE_SECTION_HEADER*)((uint8_t*)opt_hdr + file_hdr->SizeOfOptionalHeader);
    for (int i = 0; i < file_hdr->NumberOfSections; i++) {
        if (sec[i].SizeOfRawData > 0) {
            void* sec_mem = kmalloc(sec[i].VirtualSize ? sec[i].VirtualSize : sec[i].SizeOfRawData);
            if (sec_mem) {
                memset(sec_mem, 0, sec[i].VirtualSize ? sec[i].VirtualSize : sec[i].SizeOfRawData);
                memcpy(sec_mem, buffer + sec[i].PointerToRawData, sec[i].SizeOfRawData);
            }
        }
    }

    kfree(buffer);

    // Spawn process for PE binary
    process_t* proc = process_create(filepath, (void (*)(void))entry_point);
    if (!proc) {
        vga_print("[PE Loader] Error spawning PE process.\n");
        return -1;
    }

    vga_print("[PE Loader] Windows PE Process spawned successfully (PID: ");
    char pid_str[16];
    sprintf(pid_str, "%u", proc->pid);
    vga_print(pid_str);
    vga_print(").\n");

    return (int)proc->pid;
}
