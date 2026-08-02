#include "kernel/syscall.h"
#include "kernel/process.h"
#include "kernel/elf.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/video/vga.h"
#include "lib/string.h"

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
            return -1;
        case SYS_WAITPID:
            return -1;
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
        case SYS_EXECVE:
            // sys_execve(path=arg1, name=arg2)
            {
                const char* name = arg2 ? (const char*)arg2 : "user_elf";
                return (int64_t)process_create_elf(name, (const char*)arg1);
            }
        case SYS_FB_DRAW:
            // sys_fb_draw(src_pixels=arg1, width=arg2, height=arg3)
            {
                extern void put_pixel(int x, int y, uint32_t color);
                extern void swap_buffers(void);
                const uint32_t* pix = (const uint32_t*)arg1;
                int w = (int)arg2;
                int h = (int)arg3;
                if (pix && w > 0 && h > 0) {
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w; x++) {
                            put_pixel(x, y, pix[y * w + x]);
                        }
                    }
                    swap_buffers();
                }
                return 0;
            }
        case SYS_MMAP:
            return (int64_t)kmalloc((size_t)arg1);
        case SYS_SLEEP:
            {
                extern void timer_wait(uint32_t ticks);
                timer_wait((uint32_t)arg1);
                return 0;
            }
        case SYS_GETCWD:
            {
                char* buf = (char*)arg1;
                size_t sz = (size_t)arg2;
                if (buf && sz > 0) {
                    strncpy(buf, "/", sz - 1);
                }
                return 0;
            }
        case SYS_CHDIR:
            return 0;
        case SYS_UNLINK:
            {
                extern dentry_t* vfs_get_root(void);
                extern int vfs_remove(dentry_t* parent, const char* name);
                return vfs_remove(vfs_get_root(), (const char*)arg1);
            }
        case SYS_MKDIR:
            {
                extern dentry_t* vfs_get_root(void);
                extern dentry_t* vfs_mkdir(dentry_t* parent, const char* name);
                return vfs_mkdir(vfs_get_root(), (const char*)arg1) ? 0 : -1;
            }
        default:
            return -1;
    }
}

uint64_t syscall_interrupt_handler(cpu_registers_t* frame) {
    if (!frame) return 0;

    uint64_t sys_num = frame->rax;

    switch (sys_num) {
        case SYS_FORK:
            frame->rax = (uint64_t)process_fork_from_frame(frame);
            return 0;

        case SYS_WAITPID: {
            uint64_t next_rsp = 0;
            frame->rax = (uint64_t)process_waitpid_from_frame((int32_t)frame->rdi, frame, &next_rsp);
            return next_rsp;
        }

        case SYS_EXIT:
            return process_exit_from_frame((int)frame->rdi, frame);

        case SYS_YIELD:
            return schedule((uint64_t)frame);

        default:
            frame->rax = (uint64_t)syscall_handler(sys_num, frame->rdi, frame->rsi, frame->rdx);
            return 0;
    }
}
