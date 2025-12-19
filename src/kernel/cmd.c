#include "cmd.h"
#include "string.h"
#include "graphics.h"
#include "timer.h"
#include "pmm.h"

extern char terminal_buffer[];
extern int term_idx;

static int output_y = 150; // Where to print command output

void cmd_print(const char* str) {
    draw_string(15, output_y, 0xFFFF00, str);
    output_y += 10;
    if (output_y > 480) output_y = 150; // Wrap around
}

void cmd_process(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        cmd_print("Commands: help, clear, time, mem, about");
    }
    else if (strcmp(cmd, "clear") == 0) {
        output_y = 150;
        // Clear will happen on next frame redraw
    }
    else if (strcmp(cmd, "time") == 0) {
        char buf[32];
        itoa(timer_get_ticks(), buf, 10);
        cmd_print("Ticks: ");
        cmd_print(buf);
    }
    else if (strcmp(cmd, "mem") == 0) {
        char buf[64];
        uint32_t free_mem = pmm_get_free_memory() / 1024; // KB
        itoa(free_mem, buf, 10);
        cmd_print("Free Memory (KB): ");
        cmd_print(buf);
    }
    else if (strcmp(cmd, "about") == 0) {
        cmd_print("CimpleOS v0.3");
        cmd_print("By: You!");
        cmd_print("A simple hobby OS");
    }
    else if (strlen(cmd) > 0) {
        cmd_print("Unknown command. Type 'help'");
    }
    
    // Clear terminal buffer
    for (int i = 0; i < 256; i++) terminal_buffer[i] = 0;
    term_idx = 0;
}
