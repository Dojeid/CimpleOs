#include "cmd.h"
#include "lib/string.h"
#include "gui/terminal.h"
#include "kernel/timer.h"
#include "mm/pmm.h"

extern char terminal_buffer[];
extern int term_idx;

// FEATURE 1: Active terminal instance (set by keyboard handler)
terminal_instance_t* active_terminal = NULL;

// Helper to print to active terminal or fallback to global
static void cmd_print(const char* text) {
    if (active_terminal) {
        terminal_instance_print(active_terminal, text);
    } else {
        terminal_print(text);  // Fallback to global
    }
}

void cmd_process(const char* cmd) {
    if (strlen(cmd) == 0) {
        cmd_print("");
        return;
    }
    
    // Add to history
    terminal_add_to_history(cmd);
    
    if (strcmp(cmd, "help") == 0) {
        cmd_print("Available commands:");
        cmd_print("  help      - Show this help");
        cmd_print("  clear     - Clear screen");
        cmd_print("  sysinfo   - System information");
        cmd_print("  time      - Show uptime");
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
