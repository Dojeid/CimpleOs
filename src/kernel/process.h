#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "fs/vfs.h"

#define MAX_PROCESSES 16
#define PROCESS_NAME_LEN 32
#define MAX_PROCESS_FDS 32
#define PROCESS_KERNEL_STACK_SIZE 4096
#define PROCESS_WAIT_ANY (-1)

typedef struct {
    uint64_t gs, fs, es, ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) cpu_registers_t;

typedef enum {
    PROCESS_STATE_UNUSED,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} process_state_t;

typedef struct process {
    uint32_t pid;
    char name[PROCESS_NAME_LEN];
    process_state_t state;
    uint64_t stack_top;
    uint64_t saved_rsp;
    uint64_t kernel_stack_base;
    uint64_t kernel_stack_size;
    uint32_t parent_pid;
    uint32_t priority;
    uint32_t cpu_time_ms;
    int exit_code;
    int wait_pid;
    int wait_result;
    file_t* fd_table[MAX_PROCESS_FDS];
} process_t;

void process_init(void);
process_t* process_create(const char* name, void (*entry_point)(void));
process_t* process_create_elf(const char* name, const char* path);
void process_yield(void);
void process_exit(int code);
void process_list(char* buffer, uint32_t max_len);
process_t* process_get_current(void);
uint64_t schedule(uint64_t current_rsp);
int64_t process_fork_from_frame(cpu_registers_t* frame);
int64_t process_waitpid_from_frame(int32_t pid, cpu_registers_t* frame, uint64_t* next_rsp);
uint64_t process_exit_from_frame(int code, cpu_registers_t* frame);
int process_kill(uint32_t pid);

#endif // PROCESS_H
