#include "graphics.h"
#include "include/font.h"
#include "lib/string.h"
#include "mm/heap.h"
#include <stddef.h>

uint32_t* video_memory;
int screen_w, screen_h;
uint32_t* back_buffer = NULL;

void graphics_init(struct multiboot_info* mb) {
    if (mb && (mb->flags & 0x10000000) && mb->framebuffer_addr != 0) {
        video_memory = (uint32_t*)(uintptr_t)mb->framebuffer_addr;  // 64-bit safe cast
        screen_w = (int)mb->framebuffer_width;
        screen_h = (int)mb->framebuffer_height;
    } else {
        // No bootloader-provided framebuffer: render into a software buffer.
        // video_memory is set to the buffer below so swap_buffers() is safe.
        video_memory = NULL;
        screen_w = 1024;
        screen_h = 768;
    }
    
    uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    back_buffer = (uint32_t*)malloc(buffer_size);
    
    if (!back_buffer) {
        back_buffer = video_memory;
    }
    if (!video_memory) {
        video_memory = back_buffer;
    }
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
    uint8_t uc = (uint8_t)c;
    if (uc > 127) return;
    
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
