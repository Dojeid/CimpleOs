#include "kernel/idt.h"
#include "lib/io.h"
#include "gui/terminal.h"
#include "lib/string.h"
#include "gui/window_manager.h"

// --- KEYBOARD STATE ---
char terminal_buffer[256];
int term_idx = 0;
int backspace_pressed = 0;
volatile int irq_count = 0;

char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',    0,  ' ',    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, '-',    0,    0,
    0, '+',   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
extern void cmd_process(const char* cmd);

void keyboard_handler() {
    irq_count++;
    static int extended = 0;
    
    uint8_t scancode = inb(0x60);

    if (scancode >= 128) {
        // BUG FIX #14: Handle break codes properly - reset extended flag
        extended = 0;
        return;
    }

    if (scancode == 0xE0) {
        extended = 1;
        return;
    }
    
    // BUG FIX #14: Handle E1 prefix (for Pause key)
    if (scancode == 0xE1) {
        // E1 prefix - Pause key sequence
        extended = 2;
        return;
    }
    
    // BUG FIX #14: Handle break codes for E0/E1 prefixed keys
    if (extended > 0 && (scancode & 0x80)) {
        // Break code for extended key sequence
        extended = 0;
        return;
    }
    
    if (!(scancode & 0x80)) {
        if (extended == 1) {
            extended = 0;
            
            if (scancode == 0x48) {
                const char* prev = terminal_get_history_prev();
                if (prev) {
                    strcpy(terminal_buffer, prev);
                    term_idx = strlen(prev);
                }
                return;
            }
            else if (scancode == 0x50) {
                const char* next = terminal_get_history_next();
                if (next) {
                    strcpy(terminal_buffer, next);
                    term_idx = strlen(next);
                }
                return;
            }
            else if (scancode == 0x49) {
                terminal_scroll_up();
                return;
            }
            else if (scancode == 0x51) {
                terminal_scroll_down();
                return;
            }
        }
        
        char c = kbd_US[scancode];
        
        // Dispatch keypress to focused window if it registers on_keydown
        window_manager_t* wm = wm_get_state();
        window_t* focused_win = wm_get_window(wm->focused_window_id);
        if (focused_win && focused_win->on_keydown) {
            focused_win->on_keydown(focused_win, c, scancode);
            return;
        }
        
        // Fallback for Terminal shell input
        if (c == '\b') {
            if (term_idx > 0) {
                term_idx--;
                terminal_buffer[term_idx] = '\0';
                backspace_pressed = 1;
            }
        }
        else if (c == '\n') {
            terminal_buffer[term_idx] = '\0';
            
            extern terminal_instance_t* active_terminal;
            if (focused_win && focused_win->render_content == NULL && focused_win->user_data) {
                active_terminal = (terminal_instance_t*)focused_win->user_data;
                char cmd_line[300];
                cmd_line[0] = '$';
                cmd_line[1] = ' ';
                for (int i = 0; i < term_idx && i < 297; i++) {
                    cmd_line[i + 2] = terminal_buffer[i];
                }
                cmd_line[term_idx + 2] = '\0';
                terminal_instance_print(active_terminal, cmd_line);
            }
            
            cmd_process(terminal_buffer);
            active_terminal = NULL;
            term_idx = 0;
            terminal_buffer[0] = '\0';
        }
        else if (c != 0) {
            if (term_idx < 255) {
                terminal_buffer[term_idx] = c;
                term_idx++;
                terminal_buffer[term_idx] = 0;
            }
        }
    }
    
}
