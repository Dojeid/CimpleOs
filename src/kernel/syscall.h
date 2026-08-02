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
#define SYS_SENDTO  23
#define SYS_RECVFROM 24

void syscall_init(void);
int64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t syscall_interrupt_handler(cpu_registers_t* frame);

#endif // SYSCALL_H
