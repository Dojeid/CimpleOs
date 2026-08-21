#include "kernel/syscall.h"
#include "kernel/process.h"
#include "kernel/elf.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/video/vga.h"
#include "lib/string.h"

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
__attribute__((weak)) int sys_pthread_create(void* thread, const void* attr, void* (*start_routine)(void*), void* arg) { (void)thread; (void)attr; (void)start_routine; (void)arg; return -1; }
__attribute__((weak)) int sys_pthread_join(uint64_t thread, void** retval) { (void)thread; (void)retval; return -1; }
__attribute__((weak)) void sys_pthread_exit(void* retval) { (void)retval; }
__attribute__((weak)) int sys_mutex_lock(void* mutex) { (void)mutex; return 0; }
__attribute__((weak)) int sys_mutex_unlock(void* mutex) { (void)mutex; return 0; }
__attribute__((weak)) int sys_openpt(int flags) { (void)flags; return -1; }
__attribute__((weak)) int sys_ptsname_r(int fd, char* buf, size_t buflen) { (void)fd; (void)buf; (void)buflen; return -1; }
__attribute__((weak)) int sys_shm_open(const char* name, int oflag, uint32_t mode) { (void)name; (void)oflag; (void)mode; return -1; }
__attribute__((weak)) int sys_shm_unlink(const char* name) { (void)name; return 0; }
__attribute__((weak)) void* sys_sem_open(const char* name, int oflag, uint32_t mode, uint32_t value) { (void)name; (void)oflag; (void)mode; (void)value; return NULL; }
__attribute__((weak)) int sys_sem_wait(void* sem) { (void)sem; return 0; }
__attribute__((weak)) int sys_sem_post(void* sem) { (void)sem; return 0; }
__attribute__((weak)) uint32_t sys_getuid(void) { return 0; }
__attribute__((weak)) int sys_setuid(uint32_t uid) { (void)uid; return 0; }
__attribute__((weak)) uint32_t sys_getgid(void) { return 0; }

void syscall_init(void) {
    vga_print("[Syscall] System Call Dispatcher registered (Falkon + musl x86_64 ABI).\n");
}

int64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    /* Linux x86_64 ABI Syscall Dispatcher for musl compatibility */
    if (sys_num == LINUX_SYS_READ) {
        int fd = (int)arg1;
        process_t* curr = process_get_current();
        if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
        return vfs_read(curr->fd_table[fd], (uint32_t)arg2, (uint8_t*)arg3);
    }
    if (sys_num == LINUX_SYS_WRITE) {
        int fd = (int)arg1;
        process_t* curr = process_get_current();
        if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
        return vfs_write(curr->fd_table[fd], (uint32_t)arg2, (const uint8_t*)arg3);
    }
    if (sys_num == LINUX_SYS_OPEN) {
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
    if (sys_num == LINUX_SYS_CLOSE) {
        int fd = (int)arg1;
        process_t* curr = process_get_current();
        if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
        vfs_close(curr->fd_table[fd]);
        curr->fd_table[fd] = 0;
        return 0;
    }
    if (sys_num == LINUX_SYS_MMAP || sys_num == LINUX_SYS_BRK) {
        return (int64_t)kmalloc((size_t)arg1);
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

    /* Falkon-OS Legacy Syscall Dispatcher */
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
            {
                const char* name = arg2 ? (const char*)arg2 : "user_elf";
                return (int64_t)process_create_elf(name, (const char*)arg1);
            }
        case SYS_FB_DRAW:
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
                if (!buf || sz == 0) return -1;
                strncpy(buf, "/", sz);
                return 0;
            }
        case SYS_CHDIR:
            return 0;
        case SYS_UNLINK:
            {
                dentry_t* root = vfs_get_root();
                return vfs_remove(root, (const char*)arg1);
            }
        case SYS_MKDIR:
            {
                dentry_t* root = vfs_get_root();
                dentry_t* d = vfs_mkdir(root, (const char*)arg1);
                return d ? 0 : -1;
            }
        case SYS_SOCKET:
            return net_socket_create((int)arg1, (int)arg2, (int)arg3);
        case SYS_BIND:
            return net_socket_bind((int)arg1, (uint32_t)arg2, (uint16_t)arg3);
        case SYS_CONNECT:
            return net_socket_connect((int)arg1, (uint32_t)arg2, (uint16_t)arg3);
        case SYS_SENDTO:
            return net_socket_sendto((int)arg1, (const void*)arg2, (size_t)arg3, 0, 0);
        case SYS_RECVFROM:
            return net_socket_recvfrom((int)arg1, (void*)arg2, (size_t)arg3);
        case SYS_PIPE:
            return sys_pipe((int*)arg1);
        case SYS_DUP2:
            return sys_dup2((int)arg1, (int)arg2);
        case SYS_STAT:
            return sys_stat((const char*)arg1, (void*)arg2);
        case SYS_LSEEK:
            {
                int fd = (int)arg1;
                process_t* curr = process_get_current();
                if (fd < 0 || fd >= MAX_PROCESS_FDS || !curr->fd_table[fd]) return -1;
                file_t* f = curr->fd_table[fd];
                int offset = (int)arg2;
                int whence = (int)arg3;
                uint32_t fsize = f->f_inode ? f->f_inode->i_size : (f->f_dentry ? f->f_dentry->size : 0);
                if (whence == 0) f->f_pos = offset;
                else if (whence == 1) f->f_pos += offset;
                else if (whence == 2) f->f_pos = fsize + offset;
                return f->f_pos;
            }
        case SYS_IOCTL:
            return sys_ioctl((int)arg1, (uint64_t)arg2, (void*)arg3);
        case SYS_KILL:
            return sys_kill((uint32_t)arg1, (int)arg2);
        case SYS_SIGNAL:
            return sys_signal((int)arg1, (void*)arg2);
        case SYS_GETPPID:
            {
                process_t* curr = process_get_current();
                return curr ? curr->parent_pid : 0;
            }
        case SYS_UNAME:
            {
                typedef struct {
                    char sysname[65];
                    char nodename[65];
                    char release[65];
                    char version[65];
                    char machine[65];
                    char domainname[65];
                } utsname_t;
                utsname_t* u = (utsname_t*)arg1;
                if (!u) return -1;
                strncpy(u->sysname, "Falkon-OS", 65);
                strncpy(u->nodename, "falkon-os", 65);
                strncpy(u->release, "6.8.0-falkon", 65);
                strncpy(u->version, "#1 SMP PREEMPT 2026", 65);
                strncpy(u->machine, "x86_64", 65);
                strncpy(u->domainname, "(none)", 65);
                return 0;
            }
        case SYS_GETTIMEOFDAY:
            {
                typedef struct { uint64_t tv_sec; uint64_t tv_usec; } timeval_t;
                timeval_t* tv = (timeval_t*)arg1;
                if (tv) {
                    extern uint32_t timer_get_ticks(void);
                    uint32_t ticks = timer_get_ticks();
                    tv->tv_sec = ticks / 100;
                    tv->tv_usec = (ticks % 100) * 10000;
                }
                return 0;
            }
        case SYS_CLOCK_GETTIME:
            {
                typedef struct { uint64_t tv_sec; uint64_t tv_nsec; } timespec_t;
                timespec_t* ts = (timespec_t*)arg2;
                if (ts) {
                    extern uint32_t timer_get_ticks(void);
                    uint32_t ticks = timer_get_ticks();
                    ts->tv_sec = ticks / 100;
                    ts->tv_nsec = (ticks % 100) * 10000000UL;
                }
                return 0;
            }
        case SYS_NANOSLEEP:
            {
                typedef struct { uint64_t tv_sec; uint64_t tv_nsec; } timespec_t;
                const timespec_t* req = (const timespec_t*)arg1;
                if (req) {
                    uint32_t ticks = (uint32_t)(req->tv_sec * 100 + req->tv_nsec / 10000000UL);
                    extern void timer_wait(uint32_t ticks);
                    if (ticks > 0) timer_wait(ticks);
                }
                return 0;
            }
        case SYS_BRK:
            return (int64_t)kmalloc((size_t)arg1);
        case SYS_FSTAT:
            return sys_fstat((int)arg1, (void*)arg2);
        case SYS_POLL:
            return sys_poll((void*)arg1, (uint32_t)arg2, (int)arg3);
        case SYS_SELECT:
            return sys_select((int)arg1, (void*)arg2, (void*)arg3, 0, 0);
        case SYS_PTHREAD_CREATE:
            return sys_pthread_create((void*)arg1, (const void*)arg2, (void* (*)(void*))arg3, 0);
        case SYS_PTHREAD_JOIN:
            return sys_pthread_join(arg1, (void**)arg2);
        case SYS_PTHREAD_EXIT:
            sys_pthread_exit((void*)arg1);
            return 0;
        case SYS_MUTEX_LOCK:
            return sys_mutex_lock((void*)arg1);
        case SYS_MUTEX_UNLOCK:
            return sys_mutex_unlock((void*)arg1);
        case SYS_OPENPT:
            return sys_openpt((int)arg1);
        case SYS_PTSNAME:
            return sys_ptsname_r((int)arg1, (char*)arg2, (size_t)arg3);
        case SYS_SHM_OPEN:
            return sys_shm_open((const char*)arg1, (int)arg2, (uint32_t)arg3);
        case SYS_SHM_UNLINK:
            return sys_shm_unlink((const char*)arg1);
        case SYS_SEM_OPEN:
            return (int64_t)(uintptr_t)sys_sem_open((const char*)arg1, (int)arg2, (uint32_t)arg3, 0);
        case SYS_SEM_WAIT:
            return sys_sem_wait((void*)arg1);
        case SYS_SEM_POST:
            return sys_sem_post((void*)arg1);
        case SYS_GETUID:
            return sys_getuid();
        case SYS_SETUID:
            return sys_setuid((uint32_t)arg1);
        case SYS_GETGID:
            return sys_getgid();
        default:
            return -1;
    }
}

uint64_t syscall_interrupt_handler(cpu_registers_t* frame) {
    if (!frame) return 0;

    uint64_t sys_num = frame->rax;

    if (sys_num == SYS_FORK || sys_num == LINUX_SYS_FORK) {
        frame->rax = (uint64_t)process_fork_from_frame(frame);
        return 0;
    }
    if (sys_num == SYS_WAITPID || sys_num == LINUX_SYS_WAIT4) {
        uint64_t next_rsp = 0;
        frame->rax = (uint64_t)process_waitpid_from_frame((int32_t)frame->rdi, frame, &next_rsp);
        return next_rsp;
    }
    if (sys_num == SYS_EXIT || sys_num == LINUX_SYS_EXIT || sys_num == LINUX_SYS_EXIT_GROUP) {
        return process_exit_from_frame((int)frame->rdi, frame);
    }
    if (sys_num == SYS_YIELD || sys_num == LINUX_SYS_SCHED_YIELD) {
        return schedule((uint64_t)frame);
    }

    frame->rax = (uint64_t)syscall_handler(sys_num, frame->rdi, frame->rsi, frame->rdx);
    return 0;
}
