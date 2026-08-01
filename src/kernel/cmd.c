#include "cmd.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "gui/terminal.h"
#include "kernel/timer.h"
#include "kernel/process.h"
#include "fs/vfs.h"
#include "fs/ext4.h"
#include "mm/pmm.h"
#include "mm/heap.h"

extern char terminal_buffer[];
extern int term_idx;

// Active terminal instance (set by keyboard handler)
terminal_instance_t* active_terminal = NULL;

static void cmd_print(const char* text) {
    if (active_terminal) {
        terminal_instance_print(active_terminal, text);
    } else {
        terminal_print(text);
    }
}

static void int_to_str(int num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    int i = 0;
    int is_neg = 0;
    if (num < 0) {
        is_neg = 1;
        num = -num;
    }
    char temp[16];
    while (num > 0) {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (is_neg) temp[i++] = '-';
    int j = 0;
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

static dentry_t* get_target_dir(const char* path_arg, char* resolved_out) {
    const char* cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
    char resolved[256];
    vfs_resolve_path(cwd, path_arg, resolved, sizeof(resolved));
    if (resolved_out) strcpy(resolved_out, resolved);
    return vfs_lookup(resolved);
}

void cmd_process(const char* cmd) {
    if (strlen(cmd) == 0) {
        cmd_print("");
        return;
    }

    terminal_add_to_history(cmd);

    if (strcmp(cmd, "help") == 0) {
        cmd_print("Falkon-OS Shell Commands:");
        cmd_print("  cd <dir>    - Change current directory");
        cmd_print("  pwd         - Print working directory");
        cmd_print("  ls [path]   - List VFS files and directories");
        cmd_print("  cat <file>  - Display file contents");
        cmd_print("  touch <file>- Create new empty file");
        cmd_print("  mkdir <dir> - Create new directory");
        cmd_print("  rm <file>   - Remove file or directory");
        cmd_print("  write <f> <t>- Write text to file");
        cmd_print("  ps / top    - List active scheduler processes");
        cmd_print("  meminfo     - Display physical memory usage");
        cmd_print("  fetch       - Display system banner & specs");
        cmd_print("  sysinfo     - Detailed kernel status");
        cmd_print("  uname       - Show OS release details");
        cmd_print("  whoami      - Print active user session");
        cmd_print("  clear       - Clear terminal screen");
        cmd_print("  calc [expr] - Basic math calculator");
        cmd_print("  echo [text] - Print text to terminal");
        cmd_print("");
    }
    else if (strncmp(cmd, "cd", 2) == 0 && (cmd[2] == '\0' || cmd[2] == ' ')) {
        const char* target_path = (strlen(cmd) > 3) ? (cmd + 3) : "/";
        const char* current_cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
        char resolved[256];
        vfs_resolve_path(current_cwd, target_path, resolved, sizeof(resolved));

        dentry_t* target = vfs_lookup(resolved);
        if (!target) {
            cmd_print("cd: Directory not found.");
        } else if (!(target->d_inode && (target->d_inode->i_mode & 0x4000))) {
            cmd_print("cd: Target is not a directory.");
        } else {
            if (active_terminal) {
                strncpy(active_terminal->cwd, resolved, sizeof(active_terminal->cwd) - 1);
            }
        }
        cmd_print("");
    }
    else if (strcmp(cmd, "pwd") == 0) {
        const char* cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
        cmd_print(cwd);
        cmd_print("");
    }
    else if (strcmp(cmd, "ps") == 0 || strcmp(cmd, "top") == 0) {
        char ps_buf[512];
        process_list(ps_buf, sizeof(ps_buf));
        cmd_print(ps_buf);
        cmd_print("");
    }
    else if (strncmp(cmd, "ls", 2) == 0 && (cmd[2] == '\0' || cmd[2] == ' ')) {
        const char* path_arg = (strlen(cmd) > 3) ? (cmd + 3) : ".";
        char resolved[256];
        dentry_t* target = get_target_dir(path_arg, resolved);

        if (!target) {
            cmd_print("ls: Directory not found.");
        } else if (!(target->d_inode && (target->d_inode->i_mode & 0x4000))) {
            char line[128];
            sprintf(line, "[FILE] %-16s (%u B)", target->d_name, target->d_inode ? target->d_inode->i_size : 0);
            cmd_print(line);
        } else {
            char line[128];
            for (uint32_t i = 0; i < target->d_child_count; i++) {
                dentry_t* child = target->d_subdirs[i];
                if (child->d_inode && (child->d_inode->i_mode & 0x4000)) {
                    sprintf(line, "[DIR]  %s/", child->d_name);
                } else {
                    sprintf(line, "[FILE] %-16s (%u B)", child->d_name, child->d_inode ? child->d_inode->i_size : 0);
                }
                cmd_print(line);
            }
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* path_arg = cmd + 4;
        char resolved[256];
        dentry_t* file = get_target_dir(path_arg, resolved);

        if (!file) {
            cmd_print("cat: File not found.");
        } else if (!(file->d_inode && (file->d_inode->i_mode & 0x8000))) {
            cmd_print("cat: Target is a directory.");
        } else if (file->d_inode && file->d_inode->i_private && file->d_inode->i_size > 0) {
            cmd_print((const char*)file->d_inode->i_private);
        } else {
            cmd_print("[File is empty]");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "touch ", 6) == 0) {
        const char* path_arg = cmd + 6;
        char resolved[256];
        const char* cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
        vfs_resolve_path(cwd, path_arg, resolved, sizeof(resolved));

        char dir_path[256] = "/";
        char filename[64] = "";
        char* last_slash = strrchr(resolved, '/');
        if (last_slash) {
            if (last_slash == resolved) {
                strcpy(dir_path, "/");
                strcpy(filename, last_slash + 1);
            } else {
                size_t dir_len = last_slash - resolved;
                strncpy(dir_path, resolved, dir_len);
                dir_path[dir_len] = '\0';
                strcpy(filename, last_slash + 1);
            }
        } else {
            strcpy(filename, resolved);
        }

        dentry_t* parent = vfs_lookup(dir_path);
        if (!parent || !(parent->d_inode && (parent->d_inode->i_mode & 0x4000))) {
            cmd_print("touch: Target directory not found.");
        } else if (vfs_create_file(parent, filename, 0, 0)) {
            cmd_print("File created successfully.");
        } else {
            cmd_print("touch: Failed to create file.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char* path_arg = cmd + 6;
        char resolved[256];
        const char* cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
        vfs_resolve_path(cwd, path_arg, resolved, sizeof(resolved));

        char dir_path[256] = "/";
        char dirname[64] = "";
        char* last_slash = strrchr(resolved, '/');
        if (last_slash) {
            if (last_slash == resolved) {
                strcpy(dir_path, "/");
                strcpy(dirname, last_slash + 1);
            } else {
                size_t dir_len = last_slash - resolved;
                strncpy(dir_path, resolved, dir_len);
                dir_path[dir_len] = '\0';
                strcpy(dirname, last_slash + 1);
            }
        } else {
            strcpy(dirname, resolved);
        }

        dentry_t* parent = vfs_lookup(dir_path);
        if (!parent || !(parent->d_inode && (parent->d_inode->i_mode & 0x4000))) {
            cmd_print("mkdir: Target parent directory not found.");
        } else if (vfs_mkdir(parent, dirname)) {
            cmd_print("Directory created successfully.");
        } else {
            cmd_print("mkdir: Failed to create directory.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "rm ", 3) == 0) {
        const char* path_arg = cmd + 3;
        char resolved[256];
        const char* cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
        vfs_resolve_path(cwd, path_arg, resolved, sizeof(resolved));

        char dir_path[256] = "/";
        char filename[64] = "";
        char* last_slash = strrchr(resolved, '/');
        if (last_slash) {
            if (last_slash == resolved) {
                strcpy(dir_path, "/");
                strcpy(filename, last_slash + 1);
            } else {
                size_t dir_len = last_slash - resolved;
                strncpy(dir_path, resolved, dir_len);
                dir_path[dir_len] = '\0';
                strcpy(filename, last_slash + 1);
            }
        } else {
            strcpy(filename, resolved);
        }

        dentry_t* parent = vfs_lookup(dir_path);
        if (vfs_remove(parent, filename) == 0) {
            cmd_print("File/Directory removed.");
        } else {
            cmd_print("rm: File or directory not found.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "write ", 6) == 0) {
        const char* args = cmd + 6;
        char filename[64] = "";
        int i = 0;
        while (args[i] && args[i] != ' ') {
            if (i < 63) filename[i] = args[i];
            i++;
        }
        filename[i] = '\0';

        while (args[i] == ' ') i++;
        const char* content = args + i;

        char resolved[256];
        const char* cwd = (active_terminal && active_terminal->cwd[0]) ? active_terminal->cwd : "/";
        vfs_resolve_path(cwd, filename, resolved, sizeof(resolved));

        dentry_t* file = vfs_lookup(resolved);
        if (!file) {
            char dir_path[256] = "/";
            char fname[64] = "";
            char* last_slash = strrchr(resolved, '/');
            if (last_slash) {
                if (last_slash == resolved) {
                    strcpy(dir_path, "/");
                    strcpy(fname, last_slash + 1);
                } else {
                    size_t dlen = last_slash - resolved;
                    strncpy(dir_path, resolved, dlen);
                    dir_path[dlen] = '\0';
                    strcpy(fname, last_slash + 1);
                }
            }
            dentry_t* parent = vfs_lookup(dir_path);
            file = vfs_create_file(parent, fname, (const uint8_t*)content, strlen(content));
        } else {
            if (file->d_inode && file->d_inode->i_private) {
                kfree(file->d_inode->i_private);
            }
            if (file->d_inode) {
                file->d_inode->i_size = strlen(content);
                file->d_inode->i_private = kmalloc(file->d_inode->i_size + 1);
                memcpy(file->d_inode->i_private, content, file->d_inode->i_size);
                ((uint8_t*)file->d_inode->i_private)[file->d_inode->i_size] = '\0';
            }
        }

        if (file) cmd_print("Wrote data to file successfully.");
        else cmd_print("write: Failed to write to file.");
        cmd_print("");
    }
    else if (strcmp(cmd, "fetch") == 0 || strcmp(cmd, "neofetch") == 0) {
        cmd_print("   🦅  Falkon-OS v0.4 (x86_64 Enterprise)");
        cmd_print("   ----------------------------------------");
        cmd_print("   OS:       Falkon-OS 64-bit Long Mode");
        cmd_print("   Kernel:   x86_64 Custom Microkernel Architecture");
        cmd_print("   VFS:      In-Memory RAMDisk Virtual File System");
        cmd_print("   Graphics: Bochs BGA / PCI Linear Framebuffer");
        cmd_print("   Shell:    Falkon Multi-Instance POSIX Shell");
        cmd_print("");
    }
    else if (strcmp(cmd, "uname") == 0) {
        cmd_print("Falkon-OS x86_64 0.4.0-generic #1 SMP PREEMPT Falkon-OS");
        cmd_print("");
    }
    else if (strcmp(cmd, "whoami") == 0) {
        cmd_print("root@falkon-os");
        cmd_print("");
    }
    else if (strcmp(cmd, "matrix") == 0) {
        cmd_print("01100110 01100001 01101100 01101011 01101111 01101110");
        cmd_print("01000110 01000001 01001100 01001011 01001111 01001110");
        cmd_print("Wake up, Neo... Falkon-OS 64-bit Kernel Active.");
        cmd_print("System Security: Long Mode Paging Enforced.");
        cmd_print("");
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        cmd_print(cmd + 5);
        cmd_print("");
    }
    else if (strncmp(cmd, "calc ", 5) == 0) {
        const char* expr = cmd + 5;
        int a = 0, b = 0;
        char op = 0;
        int idx = 0;
        while (expr[idx] >= '0' && expr[idx] <= '9') {
            a = a * 10 + (expr[idx] - '0');
            idx++;
        }
        while (expr[idx] == ' ') idx++;
        op = expr[idx++];
        while (expr[idx] == ' ') idx++;
        while (expr[idx] >= '0' && expr[idx] <= '9') {
            b = b * 10 + (expr[idx] - '0');
            idx++;
        }
        int res = 0;
        int valid = 1;
        if (op == '+') res = a + b;
        else if (op == '-') res = a - b;
        else if (op == '*') res = a * b;
        else if (op == '/' && b != 0) res = a / b;
        else valid = 0;
        if (valid) {
            char res_str[32] = "Result: ";
            char num_str[16];
            int_to_str(res, num_str);
            strcat(res_str, num_str);
            cmd_print(res_str);
        } else {
            cmd_print("Usage: calc <num> <+|-|*|/> <num>");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "theme", 5) == 0) {
        extern void desktop_set_theme(int theme_id);
        int theme = 1;
        if (strlen(cmd) >= 7) {
            theme = cmd[6] - '0';
        }
        desktop_set_theme(theme);
        cmd_print("Desktop wallpaper theme updated!");
        cmd_print("");
    }
    else if (strcmp(cmd, "clear") == 0) {
        if (active_terminal) {
            terminal_instance_clear(active_terminal);
        } else {
            terminal_clear();
        }
    }
    else if (strcmp(cmd, "sysinfo") == 0) {
        extern void sysinfo_print();
        sysinfo_print();
    }
    else if (strcmp(cmd, "meminfo") == 0) {
        uint64_t total = pmm_get_total_memory() / (1024 * 1024);
        uint64_t free_mem = pmm_get_free_memory() / (1024 * 1024);
        uint64_t used = total - free_mem;
        char buf[128];
        sprintf(buf, "Memory: %u MB Total | %u MB Used | %u MB Free", (uint32_t)total, (uint32_t)used, (uint32_t)free_mem);
        cmd_print(buf);
        cmd_print("");
    }
    else if (strcmp(cmd, "time") == 0) {
        extern volatile uint32_t timer_ticks;
        uint32_t seconds = timer_ticks / 100;
        uint32_t minutes = seconds / 60;
        uint32_t hours = minutes / 60;
        seconds = seconds % 60;
        minutes = minutes % 60;
        char buf[64];
        sprintf(buf, "Uptime: %02u:%02u:%02u", hours, minutes, seconds);
        cmd_print(buf);
        cmd_print("");
    }
    else if (strcmp(cmd, "ext4info") == 0) {
        ext4_superblock_t* sb = ext4_get_superblock();
        if (sb && ext4_is_mounted()) {
            char buf1[128], buf2[128];
            sprintf(buf1, "EXT4 Superblock: Magic 0x%X | Volume: %s", sb->s_magic, sb->s_volume_name);
            sprintf(buf2, "Inodes: %u total | Free Blocks: %u", sb->s_inodes_count, sb->s_free_blocks_count_lo);
            cmd_print(buf1);
            cmd_print(buf2);
        } else {
            cmd_print("EXT4 Driver Engine Status: Registered (Superblock Magic 0xEF53)");
            cmd_print("Mount Target: /dev/sda | Run 'OS Installer Wizard' to deploy partition.");
        }
        cmd_print("");
    }
    else {
        cmd_print("Unknown command. Type 'help' for available commands.");
        cmd_print("");
    }

    terminal_buffer[0] = '\0';
    term_idx = 0;
    terminal_reset_history_pos();
}
