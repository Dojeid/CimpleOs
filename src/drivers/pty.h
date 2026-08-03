#ifndef PTY_H
#define PTY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTY_BUF_SIZE 2048
#define MAX_PTY_PAIRS 8

typedef struct {
    int      id;
    int      master_fd;
    int      slave_fd;
    char     slave_name[32];
    uint8_t  in_buf[PTY_BUF_SIZE];
    uint32_t in_head;
    uint32_t in_tail;
    uint8_t  out_buf[PTY_BUF_SIZE];
    uint32_t out_head;
    uint32_t out_tail;
    int      unlocked;
} pty_pair_t;

void pty_init(void);
int  posix_openpt(int flags);
int  grantpt(int fd);
int  unlockpt(int fd);
char* ptsname_r(int fd, char* buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif // PTY_H
