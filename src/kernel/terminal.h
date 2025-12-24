#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 120
#define VISIBLE_LINES 30
#define HISTORY_SIZE 50

// FEATURE 1: Terminal instance (independent state per window)
typedef struct {
    char lines[MAX_LINES][MAX_LINE_LENGTH];
    char history[HISTORY_SIZE][256];
    char input_buffer[256];
    int line_count;
    int scroll_offset;
    int history_count;
    int history_pos;
    int cursor_pos;
} terminal_instance_t;

// Legacy global terminal structure (for backwards compatibility)
typedef terminal_instance_t terminal_t;

// Instance management
terminal_instance_t* terminal_create_instance();
void terminal_destroy_instance(terminal_instance_t* term);

// Instance-based operations
void terminal_instance_init(terminal_instance_t* term);
void terminal_instance_print(terminal_instance_t* term, const char* text);
void terminal_instance_clear(terminal_instance_t* term);
void terminal_instance_render(terminal_instance_t* term, int x, int y);
void terminal_instance_scroll_up(terminal_instance_t* term);
void terminal_instance_scroll_down(terminal_instance_t* term);
void terminal_instance_add_to_history(terminal_instance_t* term, const char* cmd);
const char* terminal_instance_get_history_prev(terminal_instance_t* term);
const char* terminal_instance_get_history_next(terminal_instance_t* term);
void terminal_instance_reset_history_pos(terminal_instance_t* term);

// Legacy global functions (use default instance)
void terminal_init();
void terminal_print(const char* text);
void terminal_clear();
void terminal_render(int x, int y);
void terminal_scroll_up();
void terminal_scroll_down();
void terminal_add_to_history(const char* cmd);
const char* terminal_get_history_prev();
const char* terminal_get_history_next();
void terminal_reset_history_pos();
terminal_t* terminal_get_state();

#endif
