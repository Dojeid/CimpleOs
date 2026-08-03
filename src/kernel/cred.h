#ifndef CRED_H
#define CRED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef struct {
    uid_t uid;
    uid_t euid;
    gid_t gid;
    gid_t egid;
} posix_cred_t;

// POSIX Credentials API
uid_t sys_getuid(void);
uid_t sys_geteuid(void);
int   sys_setuid(uid_t uid);

gid_t sys_getgid(void);
gid_t sys_getegid(void);
int   sys_setgid(gid_t gid);

#ifdef __cplusplus
}
#endif

#endif // CRED_H
