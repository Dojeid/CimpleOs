#include "kernel/pthread.h"
#include "kernel/process.h"
#include "lib/printf.h"

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    process_t* proc = process_create("pthread_worker", (void(*)(void))start_routine);
    if (!proc) return -1;
    
    if (thread) *thread = proc->pid;
    printf("[Pthread] Created thread (PID %u)\n", proc->pid);
    return 0;
}

int pthread_join(pthread_t thread, void** retval) {
    process_yield();
    if (retval) *retval = 0;
    return 0;
}

void pthread_exit(void* retval) {
    process_exit(0);
}

int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr) {
    if (!mutex) return -1;
    mutex->lock = 0;
    mutex->owner_pid = 0;
    mutex->wait_count = 0;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    mutex->lock = 0;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    while (__sync_lock_test_and_set(&mutex->lock, 1)) {
        process_yield();
    }
    mutex->owner_pid = process_get_current()->pid;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    if (!mutex) return -1;
    mutex->owner_pid = 0;
    __sync_lock_release(&mutex->lock);
    return 0;
}

int pthread_cond_init(pthread_cond_t* cond, const void* attr) {
    if (!cond) return -1;
    cond->value = 0;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t* cond) {
    if (!cond) return -1;
    cond->value = 0;
    return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    pthread_mutex_unlock(mutex);
    process_yield();
    pthread_mutex_lock(mutex);
    return 0;
}

int pthread_cond_signal(pthread_cond_t* cond) {
    if (cond) cond->value++;
    return 0;
}
