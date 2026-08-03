#include "drivers/pty.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

static pty_pair_t g_ptys[MAX_PTY_PAIRS];
static int g_pty_count = 0;

void pty_init(void) {
    memset(g_ptys, 0, sizeof(g_ptys));
    for (int i = 0; i < MAX_PTY_PAIRS; i++) {
        g_ptys[i].id = -1;
    }
    vga_print("[PTY] POSIX Pseudo-Terminal Subsystem initialized.\n");
}

int posix_openpt(int flags) {
    (void)flags;
    for (int i = 0; i < MAX_PTY_PAIRS; i++) {
        if (g_ptys[i].id == -1) {
            g_ptys[i].id = i;
            g_ptys[i].master_fd = 3 + i * 2;
            g_ptys[i].slave_fd = 4 + i * 2;
            snprintf(g_ptys[i].slave_name, sizeof(g_ptys[i].slave_name), "/dev/pts/%d", i);
            g_ptys[i].unlocked = 0;
            g_pty_count++;
            return g_ptys[i].master_fd;
        }
    }
    return -1;
}

int grantpt(int fd) {
    for (int i = 0; i < MAX_PTY_PAIRS; i++) {
        if (g_ptys[i].master_fd == fd) return 0;
    }
    return -1;
}

int unlockpt(int fd) {
    for (int i = 0; i < MAX_PTY_PAIRS; i++) {
        if (g_ptys[i].master_fd == fd) {
            g_ptys[i].unlocked = 1;
            return 0;
        }
    }
    return -1;
}

char* ptsname_r(int fd, char* buf, size_t buflen) {
    if (!buf || buflen == 0) return NULL;
    for (int i = 0; i < MAX_PTY_PAIRS; i++) {
        if (g_ptys[i].master_fd == fd) {
            strncpy(buf, g_ptys[i].slave_name, buflen - 1);
            buf[buflen - 1] = '\0';
            return buf;
        }
    }
    return NULL;
}
