#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "kernel/process.h"

#define SYS_READ    1
#define SYS_WRITE   2
#define SYS_OPEN    3
#define SYS_MALLOC  4
#define SYS_FREE    5
#define SYS_GETPID  6
#define SYS_YIELD   7
#define SYS_EXIT    8
#define SYS_CLOSE   9
#define SYS_EXECVE  10
#define SYS_FB_DRAW 11
#define SYS_MMAP    12
#define SYS_FORK    13
#define SYS_WAITPID 14
#define SYS_SLEEP   15
#define SYS_GETCWD  16
#define SYS_CHDIR   17
#define SYS_UNLINK  18
#define SYS_MKDIR   19
#define SYS_SOCKET  20
#define SYS_BIND    21
#define SYS_CONNECT 22
#define SYS_SENDTO   23
#define SYS_RECVFROM 24
#define SYS_PIPE     25
#define SYS_DUP2     26
#define SYS_STAT     27
#define SYS_LSEEK    28
#define SYS_IOCTL    29
#define SYS_KILL     30
#define SYS_SIGNAL   31
#define SYS_GETPPID  32
#define SYS_UNAME    33
#define SYS_GETTIMEOFDAY  34
#define SYS_CLOCK_GETTIME 35
#define SYS_NANOSLEEP     36
#define SYS_BRK      37
#define SYS_FSTAT    38
#define SYS_POLL     39
#define SYS_SELECT   40
#define SYS_PTHREAD_CREATE 41
#define SYS_PTHREAD_JOIN   42
#define SYS_PTHREAD_EXIT   43
#define SYS_MUTEX_LOCK     44
#define SYS_MUTEX_UNLOCK   45
#define SYS_OPENPT         46
#define SYS_PTSNAME        47
#define SYS_SHM_OPEN       48
#define SYS_SHM_UNLINK     49
#define SYS_SEM_OPEN       50
#define SYS_SEM_WAIT       51
#define SYS_SEM_POST       52
#define SYS_GETUID         53
#define SYS_SETUID         54
#define SYS_GETGID         55

/* Linux x86_64 Syscall Numbers for musl compatibility */
#define LINUX_SYS_READ              0
#define LINUX_SYS_WRITE             1
#define LINUX_SYS_OPEN              2
#define LINUX_SYS_CLOSE             3
#define LINUX_SYS_STAT              4
#define LINUX_SYS_FSTAT             5
#define LINUX_SYS_LSTAT             6
#define LINUX_SYS_POLL              7
#define LINUX_SYS_LSEEK             8
#define LINUX_SYS_MMAP              9
#define LINUX_SYS_MPROTECT          10
#define LINUX_SYS_MUNMAP            11
#define LINUX_SYS_BRK               12
#define LINUX_SYS_RT_SIGACTION      13
#define LINUX_SYS_RT_SIGPROCMASK    14
#define LINUX_SYS_IOCTL             16
#define LINUX_SYS_SCHED_YIELD       24
#define LINUX_SYS_GETPID            39
#define LINUX_SYS_SOCKET            41
#define LINUX_SYS_CONNECT           42
#define LINUX_SYS_ACCEPT            43
#define LINUX_SYS_SENDTO            44
#define LINUX_SYS_RECVFROM          45
#define LINUX_SYS_CLONE             56
#define LINUX_SYS_FORK              57
#define LINUX_SYS_EXECVE            59
#define LINUX_SYS_EXIT              60
#define LINUX_SYS_WAIT4             61
#define LINUX_SYS_UNAME             63
#define LINUX_SYS_GETCWD            79
#define LINUX_SYS_CHDIR             80
#define LINUX_SYS_MKDIR             83
#define LINUX_SYS_RMDIR             84
#define LINUX_SYS_UNLINK            87
#define LINUX_SYS_GETUID            102
#define LINUX_SYS_GETGID            104
#define LINUX_SYS_ARCH_PRCTL        158
#define LINUX_SYS_SET_TID_ADDRESS   218
#define LINUX_SYS_EXIT_GROUP        231

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

void syscall_init(void);
int64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t syscall_interrupt_handler(cpu_registers_t* frame);

#endif // SYSCALL_H
