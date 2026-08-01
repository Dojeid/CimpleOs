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
        case SYS_OPEN: {
            file_t* f = vfs_open((const char*)arg1, (uint32_t)arg2);
            if (!f) return -1;
            process_t* curr = process_get_current();
            for (int i = 0; i < MAX_PROCESS_FDS; i++) {
                if (!curr->fd_table[i]) {
                    curr->fd_table[i] = f;
                    return i;
                }
            }
            vfs_close(f);
            return -1;
        }
        case SYS_READ: {
            int fd = (int)arg1;
            process_t* curr = process_get_current();
            if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
            return vfs_read(curr->fd_table[fd], (uint32_t)arg2, (uint8_t*)arg3);
        }
        case SYS_WRITE: {
            int fd = (int)arg1;
            process_t* curr = process_get_current();
            if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
            return vfs_write(curr->fd_table[fd], (uint32_t)arg2, (const uint8_t*)arg3);
        }
        case SYS_CLOSE: {
            int fd = (int)arg1;
            process_t* curr = process_get_current();
            if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
            vfs_close(curr->fd_table[fd]);
            curr->fd_table[fd] = 0;
            return 0;
        }
        case SYS_FORK:
            // TODO: Requires page table cloning or deep stack copying
            return -1;
        case SYS_EXECVE:
            // TODO: Requires ELF loader (Phase 3)
            return -1;
        case SYS_WAITPID: {
            // Block current process until child (arg1) exits
            process_t* curr = process_get_current();
            curr->state = PROCESS_STATE_BLOCKED;
            // The scheduler will skip it until unblocked
            process_yield();
            return 0;
        }
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
