#include "vga.h"

// VGA Text Mode (80x25) - 64-bit safe
uint16_t* vga_buffer = (uint16_t*)(uintptr_t)0xB8000;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;

int vga_row = 0;
int vga_col = 0;

uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_clear() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', 0x0F);
        }
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, 0x0F);
        vga_col++;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    }
    
    // BUG FIX #4: Only scroll when row actually exceeds height
    // Check at the END of the function to allow row == VGA_HEIGHT once
    if (vga_row >= VGA_HEIGHT) {
        // Scroll the screen up by one row
        for (int y = 1; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
            }
        }
        // Clear the last row
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', 0x0F);
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}
