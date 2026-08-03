#include "kernel/shm.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "kernel/process.h"
#include "lib/string.h"
#include "lib/printf.h"

static shm_node_t g_shm_table[MAX_SHM_NODES];
static sem_node_t g_sem_table[MAX_SEM_NODES];

int shm_open(const char* name, int oflag, uint32_t mode) {
    (void)oflag; (void)mode;
    if (!name) return -1;

    for (int i = 0; i < MAX_SHM_NODES; i++) {
        if (g_shm_table[i].size > 0 && strcmp(g_shm_table[i].name, name) == 0) {
            g_shm_table[i].ref_count++;
            return 100 + i;
        }
    }

    for (int i = 0; i < MAX_SHM_NODES; i++) {
        if (g_shm_table[i].size == 0) {
            strncpy(g_shm_table[i].name, name, 63);
            g_shm_table[i].name[63] = '\0';
            g_shm_table[i].size = 4096;
            g_shm_table[i].phys_addr = pmm_alloc_frame();
            g_shm_table[i].ref_count = 1;
            return 100 + i;
        }
    }
    return -1;
}

int shm_unlink(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < MAX_SHM_NODES; i++) {
        if (g_shm_table[i].size > 0 && strcmp(g_shm_table[i].name, name) == 0) {
            if (g_shm_table[i].phys_addr) pmm_free_frame(g_shm_table[i].phys_addr);
            memset(&g_shm_table[i], 0, sizeof(shm_node_t));
            return 0;
        }
    }
    return -1;
}

sem_node_t* sem_open(const char* name, int oflag, uint32_t mode, uint32_t value) {
    (void)oflag; (void)mode;
    if (!name) return NULL;

    for (int i = 0; i < MAX_SEM_NODES; i++) {
        if (g_sem_table[i].ref_count > 0 && strcmp(g_sem_table[i].name, name) == 0) {
            g_sem_table[i].ref_count++;
            return &g_sem_table[i];
        }
    }

    for (int i = 0; i < MAX_SEM_NODES; i++) {
        if (g_sem_table[i].ref_count == 0) {
            strncpy(g_sem_table[i].name, name, 63);
            g_sem_table[i].name[63] = '\0';
            g_sem_table[i].value = (int)value;
            g_sem_table[i].ref_count = 1;
            return &g_sem_table[i];
        }
    }
    return NULL;
}

int sem_close(sem_node_t* sem) {
    if (!sem) return -1;
    if (sem->ref_count > 0) sem->ref_count--;
    return 0;
}

int sem_unlink(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < MAX_SEM_NODES; i++) {
        if (g_sem_table[i].ref_count > 0 && strcmp(g_sem_table[i].name, name) == 0) {
            memset(&g_sem_table[i], 0, sizeof(sem_node_t));
            return 0;
        }
    }
    return -1;
}

int sem_wait(sem_node_t* sem) {
    if (!sem) return -1;
    while (sem->value <= 0) {
        process_yield();
    }
    __sync_fetch_and_sub(&sem->value, 1);
    return 0;
}

int sem_post(sem_node_t* sem) {
    if (!sem) return -1;
    __sync_fetch_and_add(&sem->value, 1);
    return 0;
}
