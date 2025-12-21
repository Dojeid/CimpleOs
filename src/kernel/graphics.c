#include "graphics.h"
#include "font.h"
#include "string.h"
#include "heap.h"
#include <stddef.h>

uint32_t* video_memory;
int screen_w, screen_h;

// Dynamic back buffer - allocated based on actual resolution
uint32_t* back_buffer = NULL;

void graphics_init(struct multiboot_info* mb) {
    video_memory = (uint32_t*) (uint32_t) mb->framebuffer_addr;
    screen_w = (int) mb->framebuffer_width;
    screen_h = (int) mb->framebuffer_height;
    
    // TEMPORARY: Disable dynamic allocation for debugging
    // This might be causing the VirtualBox crash
    // uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    // back_buffer = (uint32_t*)malloc(buffer_size);
    
    // Use direct rendering (no double buffering) for now
    back_buffer = video_memory;
}

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < screen_w && y >= 0 && y < screen_h) {
        back_buffer[y * screen_w + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            put_pixel(x + j, y + i, color);
}

void draw_char(int x, int y, char c, uint32_t color) {
    if (c < 0 || c > 127) return;
    
    const uint8_t *glyph = font8x8_basic[(int)c];
    
    for (int cy = 0; cy < 8; cy++) {
        for (int cx = 0; cx < 8; cx++) {
            if (glyph[cy] & (1 << (7 - cx))) {
                put_pixel(x + cx, y + cy, color);
            }
        }
    }
}

void draw_string(int x, int y, uint32_t color, const char *str) {
    int cursor_x = x;
    int cursor_y = y;
    while (*str) {
        if (*str == '\n') {
            cursor_y += 10;
            cursor_x = x;
        } else {
            draw_char(cursor_x, cursor_y, *str, color);
            cursor_x += 8;
        }
        str++;
    }
}

void swap_buffers() {
    memcpy(video_memory, back_buffer, screen_w * screen_h * 4);
}

void clear_screen(uint32_t color) {
    for(int i=0; i<screen_w*screen_h; i++) back_buffer[i] = color;
}
