#include "kernel/pipe.h"
#include "mm/heap.h"
#include "lib/string.h"

static inline void cli(void) {
    __asm__ volatile("cli");
}

static inline void sti(void) {
    __asm__ volatile("sti");
}

pipe_t* pipe_create(void) {
    pipe_t* p = (pipe_t*)kmalloc(sizeof(pipe_t));
    if (!p) return NULL;
    
    memset(p->buffer, 0, PIPE_BUF_SIZE);
    p->read_pos = 0;
    p->write_pos = 0;
    p->count = 0;
    p->read_fd_open = 1;
    p->write_fd_open = 1;
    
    return p;
}

int pipe_read(pipe_t* p, uint8_t* buf, uint32_t len) {
    if (!p || !buf || len == 0) return 0;
    
    cli();
    uint32_t read_len = (len < p->count) ? len : p->count;
    
    for (uint32_t i = 0; i < read_len; i++) {
        buf[i] = p->buffer[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUF_SIZE;
    }
    
    p->count -= read_len;
    sti();
    
    return read_len;
}

int pipe_write(pipe_t* p, const uint8_t* buf, uint32_t len) {
    if (!p || !buf || len == 0) return 0;
    
    cli();
    uint32_t available_space = PIPE_BUF_SIZE - p->count;
    uint32_t write_len = (len < available_space) ? len : available_space;
    
    for (uint32_t i = 0; i < write_len; i++) {
        p->buffer[p->write_pos] = buf[i];
        p->write_pos = (p->write_pos + 1) % PIPE_BUF_SIZE;
    }
    
    p->count += write_len;
    sti();
    
    return write_len;
}

void pipe_close_read(pipe_t* p) {
    if (!p) return;
    cli();
    p->read_fd_open = 0;
    sti();
    if (!p->read_fd_open && !p->write_fd_open) {
        pipe_destroy(p);
    }
}

void pipe_close_write(pipe_t* p) {
    if (!p) return;
    cli();
    p->write_fd_open = 0;
    sti();
    if (!p->read_fd_open && !p->write_fd_open) {
        pipe_destroy(p);
    }
}

void pipe_destroy(pipe_t* p) {
    if (p) {
        kfree(p);
    }
}

int pipe_bytes_available(pipe_t* p) {
    if (!p) return 0;
    return p->count;
}
