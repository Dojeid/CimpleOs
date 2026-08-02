#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

static process_t processes[MAX_PROCESSES];
static uint32_t current_pid = 0;
static uint32_t next_pid = 1;

void process_init(void) {
    memset(processes, 0, sizeof(processes));
    
    // Process 0: System Idle Kernel Process
    processes[0].pid = 0;
    strcpy(processes[0].name, "kernel_idle");
    processes[0].state = PROCESS_STATE_RUNNING;
    processes[0].priority = 1;
    
    vga_print("[Scheduler] Process Scheduler initialized (PID 0: kernel_idle).\n");
}

process_t* process_create(const char* name, void (*entry_point)(void)) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_STATE_TERMINATED || processes[i].pid == 0) {
            processes[i].pid = next_pid++;
            strncpy(processes[i].name, name, PROCESS_NAME_LEN - 1);
            processes[i].state = PROCESS_STATE_READY;
            processes[i].priority = 1;
            processes[i].cpu_time_ms = 0;
            return &processes[i];
        }
    }
    return 0;
}

process_t* process_create_elf(const char* name, const char* path) {
    extern vfs_node_t* vfs_lookup(vfs_node_t* relative_to, const char* path);
    extern int vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    extern int elf_validate_header(const void* buf, size_t size);
    extern uint64_t elf_load_executable(const void* buf, size_t size, uint64_t* out_entry);
    
    static char elf_buf[512 * 1024]; // 512KB VFS file buffer
    vfs_node_t* node = vfs_lookup(0, path);
    if (!node) {
        printf("[Process] ELF file not found in VFS: %s\n", path);
        return 0;
    }
    
    int sz = vfs_read(node, 0, sizeof(elf_buf), (uint8_t*)elf_buf);
    if (sz <= 0) {
        printf("[Process] Failed to read ELF file: %s\n", path);
        return 0;
    }
    
    uint64_t entry_point = 0;
    if (!elf_load_executable(elf_buf, (size_t)sz, &entry_point)) {
        printf("[Process] Failed to parse/load ELF binary: %s\n", path);
        return 0;
    }
    
    process_t* proc = process_create(name, (void(*)(void))entry_point);
    if (proc) {
        printf("[Process] Spawned Ring 3 User-Mode Process '%s' (PID %u) @ 0x%lX\n", name, proc->pid, entry_point);
    }
    return proc;
}

void process_yield(void) {
    uint32_t prev_pid = current_pid;
    uint32_t next_p = (current_pid + 1) % MAX_PROCESSES;

    for (int count = 0; count < MAX_PROCESSES; count++) {
        if (processes[next_p].pid != 0 && processes[next_p].state == PROCESS_STATE_READY) {
            if (processes[prev_pid].state == PROCESS_STATE_RUNNING) {
                processes[prev_pid].state = PROCESS_STATE_READY;
            }
            processes[next_p].state = PROCESS_STATE_RUNNING;
            current_pid = next_p;
            break;
        }
        next_p = (next_p + 1) % MAX_PROCESSES;
    }
}

void process_exit(int code) {
    processes[current_pid].state = PROCESS_STATE_TERMINATED;
    process_yield();
}

process_t* process_get_current(void) {
    return &processes[current_pid];
}

void process_list(char* buffer, uint32_t max_len) {
    if (!buffer || max_len == 0) return;
    buffer[0] = 0;

    strcat(buffer, "PID   STATE       NAME\n");
    strcat(buffer, "---   ---------   --------------------\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid != 0 || i == 0) {
            char line[64];
            const char* st_str = "UNKNOWN";
            switch (processes[i].state) {
                case PROCESS_STATE_READY:      st_str = "READY"; break;
                case PROCESS_STATE_RUNNING:    st_str = "RUNNING"; break;
                case PROCESS_STATE_BLOCKED:    st_str = "BLOCKED"; break;
                case PROCESS_STATE_TERMINATED: st_str = "STOPPED"; break;
            }
            sprintf(line, "%-5u %-11s %s\n", processes[i].pid, st_str, processes[i].name);
            if (strlen(buffer) + strlen(line) < max_len) {
                strcat(buffer, line);
            }
        }
    }
}
