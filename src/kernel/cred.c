#include "kernel/cred.h"
#include "kernel/process.h"

static posix_cred_t g_system_cred = {
    .uid = 0,    // root
    .euid = 0,   // root
    .gid = 0,    // root
    .egid = 0    // root
};

uid_t sys_getuid(void) {
    return g_system_cred.uid;
}

uid_t sys_geteuid(void) {
    return g_system_cred.euid;
}

int sys_setuid(uid_t uid) {
    if (g_system_cred.euid == 0 || uid == g_system_cred.uid) {
        g_system_cred.uid = uid;
        g_system_cred.euid = uid;
        return 0;
    }
    return -1;
}

gid_t sys_getgid(void) {
    return g_system_cred.gid;
}

gid_t sys_getegid(void) {
    return g_system_cred.egid;
}

int sys_setgid(gid_t gid) {
    if (g_system_cred.egid == 0 || gid == g_system_cred.gid) {
        g_system_cred.gid = gid;
        g_system_cred.egid = gid;
        return 0;
    }
    return -1;
}
