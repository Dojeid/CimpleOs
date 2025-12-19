#include "idt.h"
#include "io.h"

// --- KEYBOARD STATE ---
char terminal_buffer[256]; // Stores what you typed
int term_idx = 0;          // Current cursor position
int backspace_pressed = 0; // Flag for main loop

// Standard US QWERTY Map
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

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    // If key is pressed (not released)
    if (!(scancode & 0x80)) {
        char c = kbd_US[scancode];
        
        if (c == '\b') { // Handle Backspace
            if (term_idx > 0) {
                term_idx--;
                terminal_buffer[term_idx] = ' '; // Erase
                backspace_pressed = 1;
            }
        }
        else if (c != 0) { // Regular character
            if (term_idx < 255) {
                terminal_buffer[term_idx] = c;
                term_idx++;
                terminal_buffer[term_idx] = 0; // Null terminate
            }
        }
    }
    outb(0x20, 0x20); // End of Interrupt
}