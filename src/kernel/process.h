#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 16
#define PROCESS_NAME_LEN 32

typedef enum {
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
    uint32_t priority;
    uint32_t cpu_time_ms;
} process_t;

void process_init(void);
process_t* process_create(const char* name, void (*entry_point)(void));
void process_yield(void);
void process_exit(int code);
void process_list(char* buffer, uint32_t max_len);
process_t* process_get_current(void);

#endif // PROCESS_H
