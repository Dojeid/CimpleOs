#ifndef PTHREAD_H
#define PTHREAD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t pthread_t;

typedef struct {
    size_t stack_size;
    int    detach_state;
} pthread_attr_t;

typedef struct {
    volatile int lock;
    uint32_t     owner_pid;
    int          recursive_count;
} pthread_mutex_t;

typedef struct {
    volatile int state;
} pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER {0, 0, 0}
#define PTHREAD_COND_INITIALIZER  {0}

// POSIX Thread Management API
int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int pthread_join(pthread_t thread, void** retval);
void pthread_exit(void* retval);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);

// POSIX Mutex API
int pthread_mutex_init(pthread_mutex_t* mutex, void* attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);

// POSIX Condition Variable API
int pthread_cond_init(pthread_cond_t* cond, void* attr);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_destroy(pthread_cond_t* cond);

#ifdef __cplusplus
}
#endif

#endif // PTHREAD_H
