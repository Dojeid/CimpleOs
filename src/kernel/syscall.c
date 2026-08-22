#include "kernel/syscall.h"
#include "kernel/process.h"
#include "kernel/elf.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/video/vga.h"
#include "gui/terminal.h"
#include "lib/string.h"
#include "lib/printf.h"

/* Weak Stub Fallbacks for Optional Subsystem APIs */
__attribute__((weak)) int net_socket_create(int domain, int type, int protocol) { (void)domain; (void)type; (void)protocol; return -1; }
__attribute__((weak)) int net_socket_bind(int sock, uint32_t ip, uint16_t port) { (void)sock; (void)ip; (void)port; return -1; }
__attribute__((weak)) int net_socket_connect(int sock, uint32_t ip, uint16_t port) { (void)sock; (void)ip; (void)port; return -1; }
__attribute__((weak)) int net_socket_sendto(int sock, const void* buf, size_t len, uint32_t ip, uint16_t port) { (void)sock; (void)buf; (void)len; (void)ip; (void)port; return -1; }
__attribute__((weak)) int net_socket_recvfrom(int sock, void* buf, size_t len) { (void)sock; (void)buf; (void)len; return -1; }
__attribute__((weak)) int sys_pipe(int pipefd[2]) { (void)pipefd; return -1; }
__attribute__((weak)) int sys_dup2(int oldfd, int newfd) { (void)oldfd; (void)newfd; return -1; }
__attribute__((weak)) int sys_stat(const char* path, void* statbuf) { (void)path; (void)statbuf; return 0; }
__attribute__((weak)) int sys_fstat(int fd, void* statbuf) { (void)fd; (void)statbuf; return 0; }
__attribute__((weak)) int sys_ioctl(int fd, uint64_t cmd, void* arg) { (void)fd; (void)cmd; (void)arg; return 0; }
__attribute__((weak)) int sys_kill(uint32_t pid, int sig) { (void)pid; (void)sig; return 0; }
__attribute__((weak)) int sys_signal(int sig, void* handler) { (void)sig; (void)handler; return 0; }
__attribute__((weak)) int sys_poll(void* fds, uint32_t nfds, int timeout) { (void)fds; (void)nfds; (void)timeout; return 0; }
__attribute__((weak)) int sys_select(int nfds, void* readfds, void* writefds, void* exceptfds, void* timeout) { (void)nfds; (void)readfds; (void)writefds; (void)exceptfds; (void)timeout; return 0; }

static uint64_t current_heap_brk = 0x20000000;

uint64_t syscall_interrupt_handler(cpu_registers_t* regs) {
    if (!regs) return 0;
    int64_t res = syscall_handler(regs->rax, regs->rdi, regs->rsi, regs->rdx);
    regs->rax = (uint64_t)res;
    return 0;
}

void syscall_init(void) {
    vga_print("[Syscall] POSIX System Call Dispatcher Active (Falkon-OS + musl x86_64 ABI).\n");
}

int64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    /* Linux x86_64 ABI Syscall Dispatcher for musl & GNU Bash compatibility */
    if (sys_num == LINUX_SYS_READ) {
        int fd = (int)arg1;
        char* buf = (char*)arg2;
        size_t len = (size_t)arg3;
        if (fd == 0) {
            extern char terminal_buffer[];
            extern int term_idx;
            if (term_idx == 0) return 0;
            size_t copy_len = (len < (size_t)term_idx) ? len : (size_t)term_idx;
            memcpy(buf, terminal_buffer, copy_len);
            memmove(terminal_buffer, terminal_buffer + copy_len, term_idx - copy_len);
            term_idx -= (int)copy_len;
            return (int64_t)copy_len;
        }
        process_t* curr = process_get_current();
        if (!curr || fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
        return vfs_read(curr->fd_table[fd], (uint32_t)len, (uint8_t*)buf);
    }

    if (sys_num == LINUX_SYS_WRITE) {
        int fd = (int)arg1;
        const char* buf = (const char*)arg2;
        size_t len = (size_t)arg3;
        if (fd == 1 || fd == 2) {
            for (size_t i = 0; i < len; i++) {
                vga_putchar(buf[i]);
            }
            terminal_instance_t* tty = (terminal_instance_t*)terminal_get_state();
            if (tty && buf) {
                terminal_instance_print(tty, buf);
            }
            return (int64_t)len;
        }
        process_t* curr = process_get_current();
        if (!curr || fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
        return vfs_write(curr->fd_table[fd], (uint32_t)len, (const uint8_t*)buf);
    }

    if (sys_num == LINUX_SYS_OPEN) {
        file_t* f = vfs_open((const char*)arg1, (uint32_t)arg2);
        if (!f) return -1;
        process_t* curr = process_get_current();
        if (!curr) return -1;
        for (int i = 3; i < MAX_PROCESS_FDS; i++) {
            if (!curr->fd_table[i]) {
                curr->fd_table[i] = f;
                return i;
            }
        }
        vfs_close(f);
        return -1;
    }

    if (sys_num == LINUX_SYS_CLOSE) {
        int fd = (int)arg1;
        process_t* curr = process_get_current();
        if (!curr || fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
        vfs_close(curr->fd_table[fd]);
        curr->fd_table[fd] = 0;
        return 0;
    }

    if (sys_num == LINUX_SYS_BRK) {
        uint64_t target_brk = arg1;
        if (target_brk == 0 || target_brk < current_heap_brk) {
            return (int64_t)current_heap_brk;
        }
        size_t sz = (size_t)(target_brk - current_heap_brk);
        void* ptr = kmalloc(sz);
        if (!ptr) return (int64_t)current_heap_brk;
        current_heap_brk = target_brk;
        return (int64_t)current_heap_brk;
    }

    if (sys_num == LINUX_SYS_MMAP) {
        size_t length = (size_t)arg2;
        if (length == 0) return -1;
        void* ptr = kmalloc(length);
        if (!ptr) return -1;
        memset(ptr, 0, length);
        return (int64_t)(uintptr_t)ptr;
    }

    if (sys_num == LINUX_SYS_GETPID) {
        return process_get_current() ? process_get_current()->pid : 1;
    }

    if (sys_num == LINUX_SYS_SCHED_YIELD) {
        process_yield();
        return 0;
    }

    if (sys_num == LINUX_SYS_STAT || sys_num == LINUX_SYS_FSTAT || sys_num == LINUX_SYS_LSTAT) {
        return 0;
    }

    if (sys_num == LINUX_SYS_ARCH_PRCTL) {
        process_t* curr = process_get_current();
        if (arg1 == ARCH_SET_FS) {
            if (curr) curr->fs_base = arg2;
            uint32_t low = (uint32_t)arg2;
            uint32_t high = (uint32_t)(arg2 >> 32);
            asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(0xC0000100));
            return 0;
        } else if (arg1 == ARCH_GET_FS) {
            if (arg2 && curr) *(uint64_t*)arg2 = curr->fs_base;
            return 0;
        }
        return -1;
    }

    if (sys_num == LINUX_SYS_SET_TID_ADDRESS) {
        process_t* curr = process_get_current();
        if (curr) curr->tid_address = arg1;
        return curr ? curr->pid : 1;
    }

    if (sys_num == LINUX_SYS_EXIT || sys_num == LINUX_SYS_EXIT_GROUP) {
        process_exit((int)arg1);
        return 0;
    }

    /* Falkon-OS Legacy Syscall Dispatcher */
    switch (sys_num) {
        case SYS_OPEN: {
            file_t* f = vfs_open((const char*)arg1, (uint32_t)arg2);
            if (!f) return -1;
            process_t* curr = process_get_current();
            if (!curr) return -1;
            for (int i = 3; i < MAX_PROCESS_FDS; i++) {
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
            if (!curr || fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
            return vfs_read(curr->fd_table[fd], (uint32_t)arg2, (uint8_t*)arg3);
        }
        case SYS_WRITE: {
            int fd = (int)arg1;
            process_t* curr = process_get_current();
            if (!curr || fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
            return vfs_write(curr->fd_table[fd], (uint32_t)arg2, (const uint8_t*)arg3);
        }
        case SYS_CLOSE: {
            int fd = (int)arg1;
            process_t* curr = process_get_current();
            if (!curr || fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
            vfs_close(curr->fd_table[fd]);
            curr->fd_table[fd] = 0;
            return 0;
        }
        case SYS_FORK:
            return -1;
        case SYS_WAITPID:
            return -1;
        case SYS_EXECVE:
            return -1;
        case SYS_GETPID:
            return process_get_current() ? process_get_current()->pid : 1;
        case SYS_MALLOC:
            return (int64_t)kmalloc((size_t)arg1);
        case SYS_FREE:
            kfree((void*)arg1);
            return 0;
        default:
            return -1;
    }
}
