#ifndef SHM_H
#define SHM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SHM_NODES 16
#define MAX_SEM_NODES 16

typedef struct {
    char     name[64];
    size_t   size;
    void*    phys_addr;
    int      ref_count;
} shm_node_t;

typedef struct {
    char     name[64];
    volatile int value;
    int      ref_count;
} sem_node_t;

// POSIX Shared Memory API
int   shm_open(const char* name, int oflag, uint32_t mode);
int   shm_unlink(const char* name);

// POSIX Named Semaphores API
sem_node_t* sem_open(const char* name, int oflag, uint32_t mode, uint32_t value);
int         sem_close(sem_node_t* sem);
int         sem_unlink(const char* name);
int         sem_wait(sem_node_t* sem);
int         sem_post(sem_node_t* sem);

#ifdef __cplusplus
}
#endif

#endif // SHM_H
