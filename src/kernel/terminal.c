#include "terminal.h"
#include "string.h"
#include "graphics.h"
#include "heap.h"

// Global default instance for backwards compatibility
static terminal_instance_t* default_instance = NULL;

// FEATURE 1: Create new terminal instance
terminal_instance_t* terminal_create_instance() {
    terminal_instance_t* term = (terminal_instance_t*)malloc(sizeof(terminal_instance_t));
    if (!term) return NULL;
    
    terminal_instance_init(term);
    return term;
}

// FEATURE 1: Destroy terminal instance
void terminal_destroy_instance(terminal_instance_t* term) {
    if (term && term != default_instance) {
        free(term);
    }
}

// FEATURE 1: Initialize terminal instance
void terminal_instance_init(terminal_instance_t* term) {
    if (!term) return;
    
    // Clear all buffers
    for (int i = 0; i < MAX_LINES; i++) {
        term->lines[i][0] = '\0';
    }
    
    for (int i = 0; i < HISTORY_SIZE; i++) {
        term->history[i][0] = '\0';
    }
    
    term->line_count = 0;
    term->scroll_offset = 0;
    term->history_count = 0;
    term->history_pos = 0;
    term->cursor_pos = 0;
    term->input_buffer[0] = '\0';
}

// FEATURE 1: Print to specific terminal instance
void terminal_instance_print(terminal_instance_t* term, const char* text) {
    if (!term) return;
    
    if (!text || strlen(text) == 0) {
        // Print empty line
        term->lines[term->line_count % MAX_LINES][0] = '\0';
        term->line_count++;
        return;
    }
    
    // Handle long lines - wrap them
    int text_len = strlen(text);
    int pos = 0;
    
    while (pos < text_len) {
        int copy_len = text_len - pos;
        if (copy_len > MAX_LINE_LENGTH - 1) {
            copy_len = MAX_LINE_LENGTH - 1;
        }
        
        // Copy chunk
        char* dest = term->lines[term->line_count % MAX_LINES];
        for (int i = 0; i < copy_len; i++) {
            dest[i] = text[pos + i];
        }
        dest[copy_len] = '\0';
        
        term->line_count++;
        pos += copy_len;
    }
    
    // Auto-scroll to bottom when new text appears
    term->scroll_offset = 0;
}

// FEATURE 1: Clear specific terminal instance
void terminal_instance_clear(terminal_instance_t* term) {
    if (!term) return;
    
    term->line_count = 0;
    term->scroll_offset = 0;
    
    for (int i = 0; i < MAX_LINES; i++) {
        term->lines[i][0] = '\0';
    }
}

// FEATURE 1: Render specific terminal instance
void terminal_instance_render(terminal_instance_t* term, int x, int y) {
    if (!term) return;
    
    // Calculate which lines to show
    int total_lines = term->line_count;
    int start_line = total_lines - VISIBLE_LINES - term->scroll_offset;
    
    if (start_line < 0) start_line = 0;
    if (start_line > total_lines) start_line = total_lines;
    
    int line_y = y;
    int lines_drawn = 0;
    
    for (int i = start_line; i < total_lines && lines_drawn < VISIBLE_LINES; i++) {
        const char* line = term->lines[i % MAX_LINES];
        draw_string(x, line_y, 0xCCCCCC, line);
        line_y += 12;  // Line height
        lines_drawn++;
    }
}

// FEATURE 1: Scroll up in specific terminal instance
void terminal_instance_scroll_up(terminal_instance_t* term) {
    if (!term) return;
    
    int max_scroll = term->line_count - VISIBLE_LINES;
    if (max_scroll < 0) max_scroll = 0;
    
    if (term->scroll_offset < max_scroll) {
        term->scroll_offset++;
    }
}

// FEATURE 1: Scroll down in specific terminal instance
void terminal_instance_scroll_down(terminal_instance_t* term) {
    if (!term) return;
    
    if (term->scroll_offset > 0) {
        term->scroll_offset--;
    }
}

// FEATURE 1: Add to history in specific terminal instance
void terminal_instance_add_to_history(terminal_instance_t* term, const char* cmd) {
    if (!term || !cmd || strlen(cmd) == 0) return;
    
    // Don't add duplicates of the last command
    if (term->history_count > 0) {
        int last_idx = (term->history_count - 1) % HISTORY_SIZE;
        if (strcmp(term->history[last_idx], cmd) == 0) {
            term->history_pos = term->history_count;
            return;
        }
    }
    
    // Add to history
    int idx = term->history_count % HISTORY_SIZE;
    strcpy(term->history[idx], cmd);
    term->history_count++;
    term->history_pos = term->history_count;
}

// FEATURE 1: Get previous history entry
const char* terminal_instance_get_history_prev(terminal_instance_t* term) {
    if (!term || term->history_count == 0) return NULL;
    
    if (term->history_pos > 0) {
        term->history_pos--;
    }
    
    int idx = term->history_pos % HISTORY_SIZE;
    
    if (term->history_pos >= term->history_count) {
        return NULL;
    }
    
    return term->history[idx];
}

// FEATURE 1: Get next history entry
const char* terminal_instance_get_history_next(terminal_instance_t* term) {
    if (!term || term->history_count == 0) return NULL;
    
    if (term->history_pos < term->history_count) {
        term->history_pos++;
    }
    
    // If we're at the end, return empty
    if (term->history_pos >= term->history_count) {
        return "";
    }
    
    int idx = term->history_pos % HISTORY_SIZE;
    return term->history[idx];
}

// FEATURE 1: Reset history position
void terminal_instance_reset_history_pos(terminal_instance_t* term) {
    if (!term) return;
    term->history_pos = term->history_count;
}

// ========== LEGACY GLOBAL FUNCTIONS (for backwards compatibility) ==========

void terminal_init() {
    if (!default_instance) {
        default_instance = terminal_create_instance();
    }
}

void terminal_print(const char* text) {
    terminal_instance_print(default_instance, text);
}

void terminal_clear() {
    terminal_instance_clear(default_instance);
}

void terminal_render(int x, int y) {
    terminal_instance_render(default_instance, x, y);
}

void terminal_scroll_up() {
    terminal_instance_scroll_up(default_instance);
}

void terminal_scroll_down() {
    terminal_instance_scroll_down(default_instance);
}

void terminal_add_to_history(const char* cmd) {
    terminal_instance_add_to_history(default_instance, cmd);
}

const char* terminal_get_history_prev() {
    return terminal_instance_get_history_prev(default_instance);
}

const char* terminal_get_history_next() {
    return terminal_instance_get_history_next(default_instance);
}

void terminal_reset_history_pos() {
    terminal_instance_reset_history_pos(default_instance);
}

terminal_t* terminal_get_state() {
    return default_instance;
}
