#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>
#include <stddef.h>

#define PIPE_BUF_SIZE 4096

typedef struct {
    uint8_t buffer[PIPE_BUF_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    int read_fd_open;
    int write_fd_open;
} pipe_t;

pipe_t* pipe_create(void);
int pipe_read(pipe_t* p, uint8_t* buf, uint32_t len);
int pipe_write(pipe_t* p, const uint8_t* buf, uint32_t len);
void pipe_close_read(pipe_t* p);
void pipe_close_write(pipe_t* p);
void pipe_destroy(pipe_t* p);
int pipe_bytes_available(pipe_t* p);

#endif // PIPE_H
