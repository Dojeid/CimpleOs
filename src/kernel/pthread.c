#include "kernel/pthread.h"
#include "kernel/process.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    if (!thread || !start_routine) return -1;

    // Allocate thread process slot sharing page directory
    process_t* proc = process_create("posix_pthread", (void(*)(void))start_routine);
    if (!proc) return -1;

    *thread = (pthread_t)proc->pid;
    return 0;
}

int pthread_join(pthread_t thread, void** retval) {
    process_t* proc = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t* p = process_get_by_index(i);
        if (p && p->pid == (uint32_t)thread) {
            proc = p;
            break;
        }
    }
    if (!proc) return -1;

    while (proc->state != PROCESS_STATE_TERMINATED) {
        process_yield();
    }
    if (retval) *retval = NULL;
    return 0;
}

void pthread_exit(void* retval) {
    (void)retval;
    process_exit(0);
}

pthread_t pthread_self(void) {
    process_t* curr = process_get_current();
    return curr ? (pthread_t)curr->pid : 0;
}

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

// POSIX Mutex Implementation
int pthread_mutex_init(pthread_mutex_t* mutex, void* attr) {
    (void)attr;
    if (!mutex) return -1;
    mutex->lock = 0;
    mutex->owner_pid = 0;
    mutex->recursive_count = 0;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    process_t* curr = process_get_current();
    uint32_t pid = curr ? curr->pid : 1;

    while (__sync_lock_test_and_set(&mutex->lock, 1)) {
        process_yield();
    }
    mutex->owner_pid = pid;
    mutex->recursive_count++;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    if (__sync_lock_test_and_set(&mutex->lock, 1) == 0) {
        process_t* curr = process_get_current();
        mutex->owner_pid = curr ? curr->pid : 1;
        mutex->recursive_count = 1;
        return 0;
    }
    return -1;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    if (mutex->recursive_count > 0) mutex->recursive_count--;
    if (mutex->recursive_count == 0) {
        mutex->owner_pid = 0;
        __sync_lock_release(&mutex->lock);
    }
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    mutex->lock = 0;
    return 0;
}

// POSIX Condition Variable Implementation
int pthread_cond_init(pthread_cond_t* cond, void* attr) {
    (void)attr;
    if (!cond) return -1;
    cond->state = 0;
    return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    if (!cond || !mutex) return -1;
    pthread_mutex_unlock(mutex);
    while (cond->state == 0) {
        process_yield();
    }
    cond->state = 0;
    pthread_mutex_lock(mutex);
    return 0;
}

int pthread_cond_signal(pthread_cond_t* cond) {
    if (!cond) return -1;
    cond->state = 1;
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t* cond) {
    if (!cond) return -1;
    cond->state = 1;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t* cond) {
    if (!cond) return -1;
    cond->state = 0;
    return 0;
}
