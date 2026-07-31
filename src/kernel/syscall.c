#include "kernel/syscall.h"
#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/video/vga.h"

void syscall_init(void) {
    vga_print("[Syscall] System Call Dispatcher registered.\n");
}

int64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (sys_num) {
        case SYS_READ:
            return vfs_read((vfs_node_t*)arg1, (uint32_t)arg2, (uint32_t)arg3, (uint8_t*)arg1);
        case SYS_WRITE:
            return vfs_write((vfs_node_t*)arg1, 0, (uint32_t)arg2, (const uint8_t*)arg3);
        case SYS_OPEN:
            return (int64_t)vfs_lookup(0, (const char*)arg1);
        case SYS_MALLOC:
            return (int64_t)kmalloc((size_t)arg1);
        case SYS_FREE:
            kfree((void*)arg1);
            return 0;
        case SYS_GETPID:
            return process_get_current()->pid;
        case SYS_YIELD:
            process_yield();
            return 0;
        case SYS_EXIT:
            process_exit((int)arg1);
            return 0;
        default:
            return -1;
    }
}
