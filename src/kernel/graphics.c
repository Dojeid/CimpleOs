#include "graphics.h"
#include "font.h"
#include <stddef.h>

uint32_t* video_memory;
int screen_w, screen_h;
// Increased to support up to 1920x1080 (prevents overflow)
uint32_t back_buffer[1920 * 1080];

void *memcpy(void *dest, const void *src, size_t count) {
    uint32_t *d = (uint32_t *)dest;
    const uint32_t *s = (const uint32_t *)src;
    size_t n = count / 4;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void graphics_init(struct multiboot_info* mb) {
    video_memory = (uint32_t*) (uint32_t) mb->framebuffer_addr;
    screen_w = (int) mb->framebuffer_width;
    screen_h = (int) mb->framebuffer_height;
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
