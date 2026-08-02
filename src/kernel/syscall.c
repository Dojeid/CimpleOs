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
            // read(node=arg1, size=arg2, buffer=arg3)
            return vfs_read((vfs_node_t*)arg1, 0, (uint32_t)arg2, (uint8_t*)arg3);
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
        default:
            return -1;
    }
}
