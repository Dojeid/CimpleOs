#include "kernel/execve.h"
#include "kernel/elf_loader.h"
#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "lib/string.h"
#include "lib/printf.h"

int sys_execve(const char* filename, char* const argv[], char* const envp[]) {
    if (!filename) return -1;

    // Lookup VFS file node
    dentry_t* node = vfs_lookup(filename);
    if (!node || !node->data || node->size == 0) {
        printf("[EXECVE] Error: file '%s' not found or empty.\n", filename);
        return -1;
    }

    // Validate ELF header
    if (!elf_validate_header(node->data, node->size)) {
        printf("[EXECVE] Error: '%s' is not a valid 64-bit ELF binary.\n", filename);
        return -1;
    }

    uint64_t entry_point = 0;
    uint64_t loaded_entry = elf_load_executable(node->data, node->size, &entry_point);
    if (!loaded_entry) {
        printf("[EXECVE] Error loading ELF segments for '%s'.\n", filename);
        return -1;
    }

    // Create process in process table
    process_t* proc = process_create_elf(node->d_name, filename);
    if (!proc) {
        printf("[EXECVE] Error creating process for '%s'.\n", filename);
        return -1;
    }

    printf("[EXECVE] Executed '%s' (PID %u) at Ring 3 Entry 0x%lX.\n", filename, proc->pid, entry_point);
    return (int)proc->pid;
}
