#ifndef PTHREAD_H
#define PTHREAD_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t pthread_t;

typedef struct {
    uint32_t stack_size;
    int detach_state;
} pthread_attr_t;

typedef struct {
    volatile uint32_t lock;
    uint32_t owner_pid;
    uint32_t wait_count;
} pthread_mutex_t;

typedef struct {
    volatile uint32_t value;
} pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER {0, 0, 0}
#define PTHREAD_COND_INITIALIZER  {0}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int pthread_join(pthread_t thread, void** retval);
void pthread_exit(void* retval);

int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

int pthread_cond_init(pthread_cond_t* cond, const void* attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_signal(pthread_cond_t* cond);

#endif // PTHREAD_H
