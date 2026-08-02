#include "graphics.h"
#include "include/font.h"
#include "lib/string.h"
#include "lib/io.h"
#include "mm/heap.h"
#include "drivers/bus/pci.h"
#include <stddef.h>

uint32_t* video_memory = NULL;
int screen_w = 1024;
int screen_h = 768;
uint32_t* back_buffer = NULL;

static int current_theme = THEME_DARK;
static int brightness_level = 100; // 0 - 100%
static int night_light_active = 0; // 0 or 1

static void bga_write(uint16_t index, uint16_t data) {
    outw(0x01CE, index);
    outw(0x01CF, data);
}

static uint16_t bga_read(uint16_t index) {
    outw(0x01CE, index);
    return inw(0x01CF);
}

void graphics_init(struct multiboot_info* mb) {
    screen_w = 1024;
    screen_h = 768;
    video_memory = NULL;

    // 1. Check Bootloader-provided Multiboot VBE Framebuffer
    if (mb && (mb->flags & 0x10000000) && mb->framebuffer_addr != 0) {
        video_memory = (uint32_t*)(uintptr_t)mb->framebuffer_addr;
        screen_w = (int)mb->framebuffer_width;
        screen_h = (int)mb->framebuffer_height;
    }

    // 2. Hardware / PCI Auto-Discovery & Bochs BGA Initialization
    if (!video_memory) {
        uint16_t bga_id = bga_read(0);
        if (bga_id >= 0xB0C0 && bga_id <= 0xB0C6) {
            bga_write(4, 0);
            bga_write(1, 1024);
            bga_write(2, 768);
            bga_write(3, 32);
            bga_write(4, 0x01 | 0x40);
        }

        struct pci_device pci_vga;
        if (pci_find_device(0x03, 0x00, 0x00, &pci_vga) || pci_find_device(0x03, 0x80, 0x00, &pci_vga)) {
            if (pci_vga.bar0 != 0) {
                video_memory = (uint32_t*)(uintptr_t)pci_vga.bar0;
            }
        }

        if (!video_memory) {
            uint32_t* candidates[] = {
                (uint32_t*)0xFD000000,
                (uint32_t*)0xE0000000,
                (uint32_t*)0xC0000000,
                (uint32_t*)0xF0000000
            };
            for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); i++) {
                if (candidates[i] != NULL) {
                    video_memory = candidates[i];
                    break;
                }
            }
        }
    }
    
    uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    back_buffer = (uint32_t*)malloc(buffer_size);
    if (!back_buffer) back_buffer = video_memory;
    if (!video_memory) video_memory = back_buffer;
}

void graphics_set_mode(int width, int height, int bpp) {
    if (width <= 0 || height <= 0) return;
    
    uint16_t bga_id = bga_read(0);
    if (bga_id >= 0xB0C0 && bga_id <= 0xB0C6) {
        bga_write(4, 0);
        bga_write(1, (uint16_t)width);
        bga_write(2, (uint16_t)height);
        bga_write(3, (uint16_t)(bpp ? bpp : 32));
        bga_write(4, 0x01 | 0x40);
    }
    
    screen_w = width;
    screen_h = height;
    
    uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    if (back_buffer && back_buffer != video_memory) {
        free(back_buffer);
    }
    back_buffer = (uint32_t*)malloc(buffer_size);
    if (!back_buffer) back_buffer = video_memory;
    
    clear_screen(current_theme == THEME_DARK ? 0x1E1E2E : 0xF3F4F6);
    
    extern void cursor_set_screen_bounds(int w, int h);
    cursor_set_screen_bounds(screen_w, screen_h);
}

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < screen_w && y >= 0 && y < screen_h) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        if (brightness_level < 100) {
            r = (r * brightness_level) / 100;
            g = (g * brightness_level) / 100;
            b = (b * brightness_level) / 100;
        }
        
        if (night_light_active) {
            r = (r * 110 > 255) ? 255 : (r * 110 / 100);
            b = (b * 65) / 100;
        }
        
        back_buffer[y * screen_w + x] = (r << 16) | (g << 8) | b;
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if (alpha == 255) {
        draw_rect(x, y, w, h, color);
        return;
    }
    if (alpha == 0) return;
    
    uint8_t sr = (color >> 16) & 0xFF;
    uint8_t sg = (color >> 8) & 0xFF;
    uint8_t sb = color & 0xFF;
    
    for (int i = 0; i < h; i++) {
        int py = y + i;
        if (py < 0 || py >= screen_h) continue;
        for (int j = 0; j < w; j++) {
            int px = x + j;
            if (px < 0 || px >= screen_w) continue;
            
            uint32_t dst = back_buffer[py * screen_w + px];
            uint8_t dr = (dst >> 16) & 0xFF;
            uint8_t dg = (dst >> 8) & 0xFF;
            uint8_t db = dst & 0xFF;
            
            uint8_t r = (sr * alpha + dr * (255 - alpha)) / 255;
            uint8_t g = (sg * alpha + dg * (255 - alpha)) / 255;
            uint8_t b = (sb * alpha + db * (255 - alpha)) / 255;
            
            put_pixel(px, py, (r << 16) | (g << 8) | b);
        }
    }
}

void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int dx = 0, dy = 0;
            if (j < r && i < r) { dx = r - j; dy = r - i; }
            else if (j >= w - r && i < r) { dx = j - (w - r - 1); dy = r - i; }
            else if (j < r && i >= h - r) { dx = r - j; dy = i - (h - r - 1); }
            else if (j >= w - r && i >= h - r) { dx = j - (w - r - 1); dy = i - (h - r - 1); }
            
            if (dx * dx + dy * dy <= r * r) {
                put_pixel(x + j, y + i, color);
            }
        }
    }
}

void draw_rounded_rect_alpha(int x, int y, int w, int h, int r, uint32_t color, uint8_t alpha) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int dx = 0, dy = 0;
            if (j < r && i < r) { dx = r - j; dy = r - i; }
            else if (j >= w - r && i < r) { dx = j - (w - r - 1); dy = r - i; }
            else if (j < r && i >= h - r) { dx = r - j; dy = i - (h - r - 1); }
            else if (j >= w - r && i >= h - r) { dx = j - (w - r - 1); dy = i - (h - r - 1); }
            
            if (dx * dx + dy * dy <= r * r) {
                draw_rect_alpha(x + j, y + i, 1, 1, color, alpha);
            }
        }
    }
}

void draw_linear_gradient(int x, int y, int w, int h, uint32_t color_start, uint32_t color_end, int vertical) {
    uint8_t r1 = (color_start >> 16) & 0xFF, g1 = (color_start >> 8) & 0xFF, b1 = color_start & 0xFF;
    uint8_t r2 = (color_end >> 16) & 0xFF,   g2 = (color_end >> 8) & 0xFF,   b2 = color_end & 0xFF;
    
    int steps = vertical ? h : w;
    if (steps <= 0) return;
    
    for (int i = 0; i < steps; i++) {
        uint8_t r = r1 + (r2 - r1) * i / steps;
        uint8_t g = g1 + (g2 - g1) * i / steps;
        uint8_t b = b1 + (b2 - b1) * i / steps;
        uint32_t col = (r << 16) | (g << 8) | b;
        
        if (vertical) {
            draw_rect(x, y + i, w, 1, col);
        } else {
            draw_rect(x + i, y, 1, h, col);
        }
    }
}

void draw_window_shadow(int x, int y, int w, int h) {
    draw_rect_alpha(x - 6, y - 6, w + 12, h + 12, 0x000000, 35);
    draw_rect_alpha(x - 3, y - 3, w + 6, h + 6, 0x000000, 60);
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

static uint32_t frame_count_sec = 0;
static uint32_t last_fps_ticks = 0;
static int current_real_fps = 60;

void swap_buffers(void) {
    if (video_memory && back_buffer) {
        memcpy(video_memory, back_buffer, screen_w * screen_h * 4);
    }
    
    // Real Empirical Frame Rate Pacing Calculation
    extern volatile uint32_t timer_ticks;
    frame_count_sec++;
    if (timer_ticks - last_fps_ticks >= 100) {
        current_real_fps = (int)frame_count_sec;
        frame_count_sec = 0;
        last_fps_ticks = timer_ticks;
    }
}

int graphics_get_real_fps(void) {
    return current_real_fps > 0 ? current_real_fps : 60;
}

void clear_screen(uint32_t color) {
    for (int i = 0; i < screen_w * screen_h; i++) back_buffer[i] = color;
}

void graphics_set_theme(int theme) {
    current_theme = theme;
}

int graphics_get_theme(void) {
    return current_theme;
}

void graphics_set_brightness(int level_percent) {
    if (level_percent < 10) level_percent = 10;
    if (level_percent > 100) level_percent = 100;
    brightness_level = level_percent;
}

int graphics_get_brightness(void) {
    return brightness_level;
}

void graphics_set_night_light(int enabled) {
    night_light_active = enabled ? 1 : 0;
}

int graphics_get_night_light(void) {
    return night_light_active;
}
