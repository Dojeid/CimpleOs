#ifndef EXECVE_H
#define EXECVE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USER_STACK_TOP  0x7FFFF000
#define USER_STACK_SIZE 0x20000 // 128 KB User Stack

int sys_execve(const char* filename, char* const argv[], char* const envp[]);

#ifdef __cplusplus
}
#endif

#endif // EXECVE_H
