#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_READ    1
#define SYS_WRITE   2
#define SYS_OPEN    3
#define SYS_MALLOC  4
#define SYS_FREE    5
#define SYS_GETPID  6
#define SYS_YIELD   7
#define SYS_EXIT    8
#define SYS_CLOSE   9
#define SYS_FORK    10
#define SYS_EXECVE  11
#define SYS_WAITPID 12

void syscall_init(void);
int64_t syscall_handler(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

#endif // SYSCALL_H
