#include "cmd.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "gui/terminal.h"
#include "kernel/timer.h"
#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/pmm.h"

extern char terminal_buffer[];
extern int term_idx;

// Active terminal instance (set by keyboard handler)
terminal_instance_t* active_terminal = NULL;

// Helper to print to active terminal or fallback to global
static void cmd_print(const char* text) {
    if (active_terminal) {
        terminal_instance_print(active_terminal, text);
    } else {
        terminal_print(text);  // Fallback to global
    }
}

// Simple integer to ASCII string conversion helper
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

void cmd_process(const char* cmd) {
    if (strlen(cmd) == 0) {
        cmd_print("");
        return;
    }
    
    // Add to history
    terminal_add_to_history(cmd);
    
    if (strcmp(cmd, "help") == 0) {
        cmd_print("Falkon-OS Shell Commands:");
        cmd_print("  ls [path]   - List VFS files/directories");
        cmd_print("  cat [file]  - Display file contents");
        cmd_print("  touch [file]- Create new empty file");
        cmd_print("  mkdir [dir] - Create new directory");
        cmd_print("  ps          - List active processes");
        cmd_print("  meminfo     - Display physical memory usage");
        cmd_print("  fetch       - Display system banner & specs");
        cmd_print("  sysinfo     - Detailed kernel status");
        cmd_print("  uname       - Show OS release details");
        cmd_print("  whoami      - Print active user session");
        cmd_print("  clear       - Clear terminal screen");
        cmd_print("  calc        - Basic math calculator");
        cmd_print("  echo [text] - Print text back to terminal");
        cmd_print("");
    }
    else if (strcmp(cmd, "ps") == 0) {
        char ps_buf[512];
        process_list(ps_buf, sizeof(ps_buf));
        cmd_print(ps_buf);
    }
    else if (strncmp(cmd, "ls", 2) == 0) {
        const char* path = (strlen(cmd) > 3) ? (cmd + 3) : "/";
        vfs_node_t* target = vfs_lookup(0, path);
        if (!target) {
            cmd_print("ls: Directory not found.");
        } else if (target->type != VFS_DIRECTORY) {
            cmd_print(target->name);
        } else {
            char line[128];
            for (uint32_t i = 0; i < target->child_count; i++) {
                vfs_node_t* child = target->children[i];
                if (child->type == VFS_DIRECTORY) {
                    sprintf(line, "[DIR]  %s/", child->name);
                } else {
                    sprintf(line, "[FILE] %-16s (%u B)", child->name, child->size);
                }
                cmd_print(line);
            }
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* path = cmd + 4;
        vfs_node_t* file = vfs_lookup(0, path);
        if (!file) {
            cmd_print("cat: File not found.");
        } else if (file->type != VFS_FILE) {
            cmd_print("cat: Target is a directory.");
        } else if (file->data && file->size > 0) {
            cmd_print((const char*)file->data);
        } else {
            cmd_print("[File is empty]");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "touch ", 6) == 0) {
        const char* name = cmd + 6;
        vfs_node_t* root = vfs_get_root();
        if (vfs_create_file(root, name, 0, 0)) {
            cmd_print("File created successfully.");
        } else {
            cmd_print("touch: Failed to create file.");
        }
        cmd_print("");
    }
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char* name = cmd + 6;
        vfs_node_t* root = vfs_get_root();
        if (vfs_mkdir(root, name)) {
            cmd_print("Directory created successfully.");
        } else {
            cmd_print("mkdir: Failed to create directory.");
        }
        cmd_print("");
    }
    else if (strcmp(cmd, "fetch") == 0 || strcmp(cmd, "neofetch") == 0) {
        cmd_print("   🦅  Falkon-OS v0.4 (x86_64)");
        cmd_print("   -----------------------------");
        cmd_print("   OS:       Falkon-OS 64-bit Long Mode");
        cmd_print("   Kernel:   x86_64 C + NASM Assembly");
        cmd_print("   Mode:     Protected Mode + Paging Active");
        cmd_print("   Graphics: VBE Framebuffer 1024x768 32-bit");
        cmd_print("   Shell:    Falkon Interactive Terminal");
        cmd_print("");
    }
    else if (strcmp(cmd, "uname") == 0) {
        cmd_print("Falkon-OS x86_64 0.4.0-generic Long_Mode GNU/Falkon");
        cmd_print("");
    }
    else if (strcmp(cmd, "whoami") == 0) {
        cmd_print("root@falkon-os");
        cmd_print("");
    }
    else if (strcmp(cmd, "matrix") == 0) {
        cmd_print("\033[32m01100110 01100001 01101100 01101011 01101111 01101110");
        cmd_print("01000110 01000001 01001100 01001011 01001111 01001110");
        cmd_print("Wake up, Neo... Falkon-OS has you.");
        cmd_print("System Security: Long Mode Paging Enforced.");
        cmd_print("");
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        cmd_print(cmd + 5);
        cmd_print("");
    }
    else if (strncmp(cmd, "calc ", 5) == 0) {
        // Simple calculator logic (e.g. calc 10 + 20)
        const char* expr = cmd + 5;
        int a = 0, b = 0;
        char op = 0;
        
        // Parse "a op b"
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
    else if (strcmp(cmd, "time") == 0) {
        extern volatile uint32_t timer_ticks;
        uint32_t seconds = timer_ticks / 100;
        uint32_t minutes = seconds / 60;
        uint32_t hours = minutes / 60;
        seconds = seconds % 60;
        minutes = minutes % 60;
        
        char buf[32];
        buf[0] = 'U'; buf[1] = 'p'; buf[2] = 't'; buf[3] = 'i'; buf[4] = 'm'; buf[5] = 'e'; buf[6] = ':'; buf[7] = ' ';
        int idx = 8;
        if (hours > 0) {
            if (hours >= 10) buf[idx++] = '0' + (hours / 10);
            buf[idx++] = '0' + (hours % 10);
            buf[idx++] = 'h'; buf[idx++] = ' ';
        }
        if (minutes >= 10) buf[idx++] = '0' + (minutes / 10);
        buf[idx++] = '0' + (minutes % 10);
        buf[idx++] = 'm'; buf[idx++] = ' ';
        if (seconds >= 10) buf[idx++] = '0' + (seconds / 10);
        buf[idx++] = '0' + (seconds % 10);
        buf[idx++] = 's';
        buf[idx] = '\0';
        cmd_print(buf);
        cmd_print("");
    }
    else {
        cmd_print("Unknown command. Type 'help' for available commands.");
        cmd_print("");
    }
    
    // Reset input
    terminal_buffer[0] = '\0';
    term_idx = 0;
    
    // Reset history position
    terminal_reset_history_pos();
}
