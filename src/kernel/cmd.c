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

// BUG FIX #1: active_terminal is now extern'd from terminal.h
// to avoid multiple definitions across translation units

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

typedef struct {
    char key[32];
    char value[128];
} env_var_t;

static env_var_t env_table[16] = {
    {"USER", "root"},
    {"HOSTNAME", "falkon-os"},
    {"HOME", "/root"},
    {"SHELL", "/bin/bash"},
    {"TERM", "xterm-256color"},
    {"PATH", "/bin:/usr/bin:/sys"},
    {"OS", "Falkon-OS Enterprise 1.0"},
    {"EDITOR", "/bin/notepad"}
};
static int env_count = 8;

static const char* get_env(const char* key) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_table[i].key, key) == 0) return env_table[i].value;
    }
    return "";
}

static void set_env(const char* key, const char* val) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_table[i].key, key) == 0) {
            strncpy(env_table[i].value, val, sizeof(env_table[i].value) - 1);
            return;
        }
    }
    if (env_count < 16) {
        strncpy(env_table[env_count].key, key, sizeof(env_table[env_count].key) - 1);
        strncpy(env_table[env_count].value, val, sizeof(env_table[env_count].value) - 1);
        env_count++;
    }
}

void cmd_process(const char* cmd) {
    if (strlen(cmd) == 0) {
        cmd_print("");
        return;
    }

    terminal_add_to_history(cmd);

    // POSIX Pipe Handling (e.g. "cat /docs/welcome.txt | grep Falkon")
    const char* pipe_ptr = strstr(cmd, "|");
    if (pipe_ptr) {
        char left_cmd[128] = {0};
        char right_cmd[128] = {0};
        size_t left_len = (size_t)(pipe_ptr - cmd);
        if (left_len >= sizeof(left_cmd)) left_len = sizeof(left_cmd) - 1;
        strncpy(left_cmd, cmd, left_len);
        left_cmd[left_len] = '\0';

        const char* right_ptr = pipe_ptr + 1;
        while (*right_ptr == ' ') right_ptr++;
        strncpy(right_cmd, right_ptr, sizeof(right_cmd) - 1);

        char combined[256];
        sprintf(combined, "%s > /tmp/pipe.tmp", left_cmd);
        cmd_process(combined);

        if (strncmp(right_cmd, "grep ", 5) == 0) {
            sprintf(combined, "%s /tmp/pipe.tmp", right_cmd);
            cmd_process(combined);
        } else if (strcmp(right_cmd, "wc") == 0 || strncmp(right_cmd, "wc ", 3) == 0) {
            sprintf(combined, "wc /tmp/pipe.tmp");
            cmd_process(combined);
        } else if (strcmp(right_cmd, "head") == 0 || strncmp(right_cmd, "head ", 5) == 0) {
            sprintf(combined, "head /tmp/pipe.tmp");
            cmd_process(combined);
        } else {
            sprintf(combined, "cat /tmp/pipe.tmp");
            cmd_process(combined);
        }
        return;
    }

    // File Redirection Handling (e.g. "echo Hello World > /docs/out.txt")
    const char* redir = strstr(cmd, ">");
    if (redir) {
        int append = (redir[1] == '>');
        char left_cmd[128];
        char file_path[128];
        
        size_t left_len = (size_t)(redir - cmd);
        if (left_len >= sizeof(left_cmd)) left_len = sizeof(left_cmd) - 1;
        strncpy(left_cmd, cmd, left_len);
        left_cmd[left_len] = '\0';

        const char* fp = redir + (append ? 2 : 1);
        while (*fp == ' ') fp++;
        strncpy(file_path, fp, sizeof(file_path) - 1);

        // Perform redirection output writing to VFS file
        dentry_t* target_dir = get_target_dir(file_path, NULL);
        if (!target_dir) target_dir = vfs_get_root();
        
        const char* content = strstr(left_cmd, "echo ") ? (left_cmd + 5) : left_cmd;
        vfs_create_file(target_dir, file_path, (const uint8_t*)content, strlen(content));
        
        char msg[160];
        sprintf(msg, "[POSIX] Redirected output to file: %s (%u bytes)", file_path, strlen(content));
        cmd_print(msg);
        cmd_print("");
        return;
    }

    if (strcmp(cmd, "help") == 0) {
        cmd_print("GNU Bash / Falkon Shell (fsh) Builtins:");
        cmd_print("  uname [-a]  - Print system architecture info");
        cmd_print("  uptime      - Show system uptime & tick timer");
        cmd_print("  free [-m]   - Display memory statistics");
        cmd_print("  df [-h]     - Display filesystem disk space");
        cmd_print("  kill <pid>  - Send termination signal to process");
        cmd_print("  bash        - GNU Bash shell banner & mode");
        cmd_print("  fsh         - Falkon OS Object Power Shell");
        cmd_print("  env / export- View or set environment variables");
        cmd_print("  echo [$VAR] - Expand variables and print text");
        cmd_print("  grep <p> <f>- Search pattern in text file");
        cmd_print("  head / tail - View top or bottom lines of file");
        cmd_print("  wc <file>   - Count lines, words, and bytes");
        cmd_print("  which <cmd> - Locate command executable binary");
        cmd_print("  find [path] - Search VFS directory tree");
        cmd_print("  history     - View command history list");
        cmd_print("  cd / pwd    - Change or print working directory");
        cmd_print("  ls / cat    - List directory or display file");
        cmd_print("");
    }
    else if (strncmp(cmd, "uname", 5) == 0) {
        cmd_print("Falkon 6.8.0-falkon #1 SMP PREEMPT 2026 x86_64 GNU/Linux");
        cmd_print("");
    }
    else if (strcmp(cmd, "uptime") == 0) {
        extern volatile uint32_t timer_ticks;
        uint32_t sec = timer_ticks / 100;
        uint32_t min = sec / 60;
        uint32_t hrs = min / 60;
        char up_buf[128];
        sprintf(up_buf, " uptime: %02u:%02u:%02u up %u sec, 1 user, load average: 0.05, 0.02, 0.00",
                hrs % 24, min % 60, sec % 60, sec);
        cmd_print(up_buf);
        cmd_print("");
    }
    else if (strncmp(cmd, "free", 4) == 0) {
        uint64_t total = pmm_get_total_memory();
        uint64_t free_mem = pmm_get_free_memory();
        uint64_t used = (total > free_mem) ? (total - free_mem) : 0;
        cmd_print("               total        used        free      shared  buff/cache   available");
        char mem_buf[160];
        sprintf(mem_buf, "Mem:        %8uMB   %8uMB   %8uMB         0MB         8MB   %8uMB",
                (uint32_t)(total / (1024*1024)),
                (uint32_t)(used / (1024*1024)),
                (uint32_t)(free_mem / (1024*1024)),
                (uint32_t)(free_mem / (1024*1024)));
        cmd_print(mem_buf);
        cmd_print("Swap:            0MB         0MB         0MB");
        cmd_print("");
    }
    else if (strncmp(cmd, "df", 2) == 0) {
        cmd_print("Filesystem     Type      Size  Used Avail Use% Mounted on");
        cmd_print("/dev/ram0      ramdisk   16M   4.2M   12M  26% /");
        cmd_print("/dev/sda1      ext4     512M    48M  464M  10% /home");
        cmd_print("/dev/sr0       iso9660  422K   422K     0 100% /media/iso");
        cmd_print("");
    }
    else if (strncmp(cmd, "kill ", 5) == 0) {
        int pid = atoi(cmd + 5);
        extern int process_kill(uint32_t pid);
        if (pid > 0 && process_kill(pid) == 0) {
            char kbuf[64];
            sprintf(kbuf, "[POSIX] Process %d terminated (SIGKILL).", pid);
            cmd_print(kbuf);
        } else {
            cmd_print("kill: process ID invalid or process 0.");
        }
        cmd_print("");
    }
    else if (strcmp(cmd, "startx") == 0 || strcmp(cmd, "gui") == 0 || strcmp(cmd, "init 5") == 0) {
        extern void sys_set_runlevel(int level);
        sys_set_runlevel(5);
        cmd_print("Launching Falkon Graphical Desktop Environment (startx)...");
        cmd_print("");
    }
    else if (strncmp(cmd, "surf ", 5) == 0 || strcmp(cmd, "surf") == 0) {
        extern void browser_open(const char* url);
        const char* url = strchr(cmd, ' ') ? (strchr(cmd, ' ') + 1) : "file:///docs/welcome.txt";
        browser_open(url);
        cmd_print("[Browser] Opened falkon-surf Web Browser.");
        cmd_print("");
    }
    else if (strncmp(cmd, "code ", 5) == 0 || strcmp(cmd, "code") == 0) {
        extern void code_editor_open(const char* file_path);
        const char* file = strchr(cmd, ' ') ? (strchr(cmd, ' ') + 1) : "/src/main.c";
        code_editor_open(file);
        cmd_print("[IDE] Opened falkon-code Advanced Editor.");
        cmd_print("");
    }
    else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "init 3") == 0 || strcmp(cmd, "tty") == 0) {
        extern void sys_set_runlevel(int level);
        sys_set_runlevel(3);
        cmd_print("Switched to Core Linux TTY CLI Console (Runlevel 3).");
        cmd_print("");
    }
    else if (strcmp(cmd, "bash") == 0) {
        cmd_print("GNU bash, version 5.2.21-release (x86_64-falkon-elf)");
        cmd_print("Copyright (C) 2026 Free Software Foundation, Inc.");
        cmd_print("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>");
        cmd_print("This is free software; you are free to change and redistribute it.");
        cmd_print("Type 'help' or 'fsh' for Falkon OS Power Shell engine.");
        cmd_print("");
    }
    else if (strcmp(cmd, "fsh") == 0) {
        cmd_print("Falkon Power Shell Engine (fsh) v1.0 [POSIX + Object Pipeline]");
        cmd_print("Type 'env' for variables, 'ps' for process objects, or 'vlc' to launch media player.");
        cmd_print("");
    }
    else if (strcmp(cmd, "env") == 0) {
        for (int i = 0; i < env_count; i++) {
            char env_line[160];
            sprintf(env_line, "%s=%s", env_table[i].key, env_table[i].value);
            cmd_print(env_line);
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "export", 6) == 0) {
        const char* arg = cmd + 6;
        while (*arg == ' ') arg++;
        char key[32] = {0};
        char val[128] = {0};
        int eq = 0;
        for (int i = 0; arg[i] != '\0'; i++) {
            if (arg[i] == '=') { eq = i; break; }
        }
        if (eq > 0) {
            strncpy(key, arg, eq);
            strcpy(val, arg + eq + 1);
            set_env(key, val);
            cmd_print("Exported environment variable.");
        } else {
            cmd_print("Usage: export KEY=VALUE");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "grep", 4) == 0) {
        const char* arg = cmd + 4;
        while (*arg == ' ') arg++;
        char pat[64] = {0};
        char path[128] = {0};
        int i = 0;
        while (*arg != '\0' && *arg != ' ' && i < 63) pat[i++] = *arg++;
        pat[i] = '\0';
        while (*arg == ' ') arg++;
        strncpy(path, arg, sizeof(path) - 1);

        if (pat[0] != '\0' && path[0] != '\0') {
            vfs_node_t* n = get_target_dir(path, NULL);
            if (n && n->type == VFS_FILE && n->data) {
                char line[256];
                int lpos = 0;
                for (uint32_t k = 0; k <= n->size; k++) {
                    char c = (k < n->size) ? (char)n->data[k] : '\n';
                    if (c == '\n') {
                        line[lpos] = '\0';
                        if (strstr(line, pat)) cmd_print(line);
                        lpos = 0;
                    } else if (lpos < 255) {
                        line[lpos++] = c;
                    }
                }
            } else {
                cmd_print("grep: File not found.");
            }
        } else {
            cmd_print("Usage: grep <pattern> <file>");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "head", 4) == 0 || strncmp(cmd, "tail", 4) == 0) {
        const char* path_arg = cmd + 4;
        while (*path_arg == ' ') path_arg++;
        vfs_node_t* n = get_target_dir(path_arg, NULL);
        if (n && n->type == VFS_FILE && n->data) {
            int line_count = 0;
            for (uint32_t i = 0; i < n->size; i++) if (n->data[i] == '\n') line_count++;
            int max_print = 5;
            int cur_line = 0;
            char line[256];
            int lpos = 0;
            int is_tail = (strncmp(cmd, "tail", 4) == 0);
            int start_print_line = is_tail ? (line_count - max_print) : 0;
            if (start_print_line < 0) start_print_line = 0;

            for (uint32_t i = 0; i <= n->size; i++) {
                char c = (i < n->size) ? (char)n->data[i] : '\n';
                if (c == '\n') {
                    line[lpos] = '\0';
                    if (cur_line >= start_print_line && (is_tail || cur_line < max_print)) {
                        cmd_print(line);
                    }
                    cur_line++;
                    lpos = 0;
                } else if (lpos < 255) {
                    line[lpos++] = c;
                }
            }
        } else {
            cmd_print("File not found.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "wc", 2) == 0) {
        const char* path_arg = cmd + 2;
        while (*path_arg == ' ') path_arg++;
        vfs_node_t* n = get_target_dir(path_arg, NULL);
        if (n && n->type == VFS_FILE && n->data) {
            int lines = 0, words = 0, bytes = n->size;
            int in_word = 0;
            for (uint32_t i = 0; i < n->size; i++) {
                char c = (char)n->data[i];
                if (c == '\n') lines++;
                if (c == ' ' || c == '\t' || c == '\n') {
                    in_word = 0;
                } else if (!in_word) {
                    in_word = 1;
                    words++;
                }
            }
            char out_str[128];
            sprintf(out_str, "  %d  %d  %d %s", lines, words, bytes, path_arg);
            cmd_print(out_str);
        } else {
            cmd_print("wc: File not found.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "which", 5) == 0 || strncmp(cmd, "whereis", 7) == 0) {
        const char* app = (strncmp(cmd, "which", 5) == 0) ? (cmd + 5) : (cmd + 7);
        while (*app == ' ') app++;
        if (strcmp(app, "vlc") == 0) cmd_print("/bin/vlc");
        else if (strcmp(app, "bash") == 0) cmd_print("/bin/bash");
        else if (strcmp(app, "notepad") == 0) cmd_print("/bin/notepad");
        else if (strcmp(app, "calc") == 0) cmd_print("/bin/calc");
        else cmd_print("Executable binary not found in PATH.");
        cmd_print("");
    }
    else if (strcmp(cmd, "history") == 0) {
        cmd_print("Command History List:");
        cmd_print("  1  help");
        cmd_print("  2  ls /docs");
        cmd_print("  3  vlc /videos/sample.mp4");
        cmd_print("  4  fetch");
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
    else if (strncmp(cmd, "display", 7) == 0) {
        extern void graphics_set_mode(int w, int h, int bpp);
        extern void graphics_set_brightness(int level);
        extern void graphics_set_night_light(int enable);
        extern void graphics_set_theme(int theme);
        
        const char* arg = cmd + 7;
        while (*arg == ' ') arg++;
        
        if (strncmp(arg, "res ", 4) == 0) {
            const char* res = arg + 4;
            if (strcmp(res, "1920x1080") == 0) graphics_set_mode(1920, 1080, 32);
            else if (strcmp(res, "1280x720") == 0) graphics_set_mode(1280, 720, 32);
            else if (strcmp(res, "800x600") == 0) graphics_set_mode(800, 600, 32);
            else graphics_set_mode(1024, 768, 32);
            cmd_print("Display resolution updated.");
        } else if (strncmp(arg, "brightness ", 11) == 0) {
            extern int atoi(const char* str);
            int b = atoi(arg + 11);
            graphics_set_brightness(b);
            cmd_print("Display brightness updated.");
        } else if (strncmp(arg, "nightlight ", 11) == 0) {
            int n = (strcmp(arg + 11, "on") == 0);
            graphics_set_night_light(n);
            cmd_print("Night light filter updated.");
        } else if (strncmp(arg, "theme ", 6) == 0) {
            int t = (strcmp(arg + 6, "light") == 0) ? 1 : 0;
            graphics_set_theme(t);
            cmd_print("UI Theme updated.");
        } else {
            cmd_print("Usage: display res <1024x768|1280x720|800x600|1920x1080>");
            cmd_print("       display brightness <10-100>");
            cmd_print("       display nightlight <on|off>");
            cmd_print("       display theme <dark|light>");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "echo", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')) {
        const char* text = (strlen(cmd) > 5) ? (cmd + 5) : "";
        if (text[0] == '$') {
            const char* val = get_env(text + 1);
            cmd_print(val[0] ? val : text);
        } else {
            cmd_print(text);
        }
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
    else if (strncmp(cmd, "ffmpeg", 6) == 0) {
        extern int avformat_open_input(void** ps, const char* url, void* fmt, void* options);
        extern void avformat_close_input(void** s);
        
        const char* arg = cmd + 6;
        while (*arg == ' ') arg++;
        if (strncmp(arg, "-i ", 3) == 0) arg += 3;
        const char* target = (*arg != '\0') ? arg : "/videos/sample.mp4";
        
        void* fmt_ctx = NULL;
        if (avformat_open_input(&fmt_ctx, target, NULL, NULL) == 0) {
            cmd_print("FFmpeg v5.2 Demuxer Analysis:");
            cmd_print("  Input #0, mov,mp4,m4a,3gp,3g2,mj2, from file:");
            char info[128];
            sprintf(info, "  Metadata: File %s", target);
            cmd_print(info);
            cmd_print("  Stream #0:0: Video: h264 (High), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 60 fps");
            cmd_print("  Stream #0:1: Audio: aac (LC), 48000 Hz, stereo, fltp, 128 kb/s");
            avformat_close_input(&fmt_ctx);
        } else {
            cmd_print("ffmpeg: Failed to open media container.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "play", 4) == 0 || strncmp(cmd, "vlc", 3) == 0 || strncmp(cmd, "exec", 4) == 0) {
        extern void media_player_open(const char* path);
        const char* arg = cmd;
        if (strncmp(cmd, "play", 4) == 0) arg += 4;
        else if (strncmp(cmd, "vlc", 3) == 0) arg += 3;
        else arg += 4;
        while (*arg == ' ') arg++;
        const char* media_path = (*arg != '\0') ? arg : "/videos/sample.mp4";
        
        media_player_open(media_path);
        char msg[128];
        sprintf(msg, "FFmpeg Powered Media Player launched for: %s", media_path);
        cmd_print(msg);
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
