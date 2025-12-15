#include <stddef.h>
#include <stdint.h>

// This function doesn't need to change
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

// This function doesn't need to change
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x7; // Light Grey on Black
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = (uint16_t) ' ' | (uint16_t) terminal_color << 8;
        }
    }
}

// --- THIS IS THE UPDATED FUNCTION ---
void terminal_putchar(char c) {
    // Handle newline characters
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        // Print all other characters
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        terminal_buffer[index] = (uint16_t) c | (uint16_t) terminal_color << 8;
        terminal_column++;
    }

    // Wrap to the next line if we've reached the end
    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
    }

    // Reset to the top if we've reached the bottom of the screen
    // (A proper scroll implementation is a future step)
    if (terminal_row >= VGA_HEIGHT) {
        terminal_row = 0;
    }
}

// This function doesn't need to change
void terminal_writestring(const char* data) {
    size_t len = strlen(data);
    for (size_t i = 0; i < len; i++)
        terminal_putchar(data[i]);
}