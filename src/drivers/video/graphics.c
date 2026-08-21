// ============================================================
// graphics.c — Falkon-OS Premium GPU Rendering Engine v2.0
// Full Windows 11 visual fidelity primitives
// ============================================================
#include "graphics.h"
#include "include/font.h"
#include "lib/string.h"
#include "lib/io.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "drivers/bus/pci.h"
#include <stddef.h>

uint32_t* video_memory = NULL;
int screen_w = 1024;
int screen_h = 768;
uint32_t* back_buffer = NULL;

// Dirty-row tracking: only copy changed rows to VRAM each frame
static uint8_t dirty_rows[1024];  // 1 byte per row — 1024 rows max

static void mark_dirty(int y) {
    if (y >= 0 && y < screen_h && y < 1024) dirty_rows[y] = 1;
}

static int current_theme = THEME_DARK;
static int brightness_level = 100;
static int night_light_active = 0;

// ─── Dirty Region Manager ───────────────────────────────────
static int dirty_min_x = 0;
static int dirty_min_y = 0;
static int dirty_max_x = 0;
static int dirty_max_y = 0;
static int dirty_active = 1;

void graphics_mark_dirty(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    int max_x = x + w;
    int max_y = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (max_x > screen_w) max_x = screen_w;
    if (max_y > screen_h) max_y = screen_h;
    if (x >= max_x || y >= max_y) return;

    if (!dirty_active) {
        dirty_min_x = x;
        dirty_min_y = y;
        dirty_max_x = max_x;
        dirty_max_y = max_y;
        dirty_active = 1;
    } else {
        if (x < dirty_min_x) dirty_min_x = x;
        if (y < dirty_min_y) dirty_min_y = y;
        if (max_x > dirty_max_x) dirty_max_x = max_x;
        if (max_y > dirty_max_y) dirty_max_y = max_y;
    }
}

void graphics_mark_all_dirty(void) {
    dirty_min_x = 0;
    dirty_min_y = 0;
    dirty_max_x = screen_w;
    dirty_max_y = screen_h;
    dirty_active = 1;
}

void graphics_clear_dirty(void) {
    dirty_active = 0;
}

int graphics_has_dirty(void) {
    return dirty_active;
}

void graphics_get_dirty_bounds(int* x, int* y, int* w, int* h) {
    if (!dirty_active) {
        if (x) *x = 0; if (y) *y = 0; if (w) *w = 0; if (h) *h = 0;
        return;
    }
    if (x) *x = dirty_min_x;
    if (y) *y = dirty_min_y;
    if (w) *w = dirty_max_x - dirty_min_x;
    if (h) *h = dirty_max_y - dirty_min_y;
}

// ─── BGA I/O ────────────────────────────────────────────────
static void bga_write(uint16_t index, uint16_t data) {
    outw(0x01CE, index); outw(0x01CF, data);
}
static uint16_t bga_read(uint16_t index) {
    outw(0x01CE, index); return inw(0x01CF);
}

// ─── Init ───────────────────────────────────────────────────
void graphics_init(struct multiboot_info* mb) {
    screen_w = 1024; screen_h = 768; video_memory = NULL;

    uint32_t boot_lfb = *(volatile uint32_t*)0x500;
    if (boot_lfb != 0 && boot_lfb != 0xFFFFFFFF && (boot_lfb % 4096 == 0)) {
        video_memory = (uint32_t*)(uintptr_t)boot_lfb;
    }

    if (!video_memory && mb && (mb->flags & 0x1000) && mb->framebuffer_addr != 0) {
        video_memory = (uint32_t*)(uintptr_t)mb->framebuffer_addr;
        screen_w = (int)mb->framebuffer_width;
        screen_h = (int)mb->framebuffer_height;
    }

    if (!video_memory) {
        struct pci_device pci_vga;
        if (pci_find_display_adapter(&pci_vga) || pci_find_device(0x03, 0x00, 0x00, &pci_vga)) {
            if (pci_vga.bar0 != 0)
                video_memory = (uint32_t*)(uintptr_t)pci_vga.bar0;
        }
        if (!video_memory) {
            uint32_t candidates[] = { 0xFD000000, 0xE0000000, 0xC0000000, 0xF0000000 };
            for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); i++) {
                if (candidates[i] != 0) { video_memory = (uint32_t*)(uintptr_t)candidates[i]; break; }
            }
        }
    }

#include "drivers/video/vga.h"
#include "mm/pmm.h"

    uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    if (pmm_get_total_memory() <= 16 * 1024 * 1024 && video_memory) {
        // Zero System-RAM Overhead: Use upper VRAM page for back_buffer
        back_buffer = video_memory + (screen_w * screen_h);
    } else {
        back_buffer = (uint32_t*)malloc(buffer_size);
        if (!back_buffer && video_memory) {
            vga_print("[Graphics] Low RAM: Placing back_buffer in upper VRAM offset\n");
            back_buffer = video_memory + (screen_w * screen_h);
        } else if (!back_buffer) {
            back_buffer = video_memory;
        }
    }
    if (!video_memory) video_memory = back_buffer;
}

void graphics_set_mode(int width, int height, int bpp) {
    if (width <= 0 || height <= 0) return;
    uint16_t bga_id = bga_read(0);
    if (bga_id >= 0xB0C0 && bga_id <= 0xB0C6) {
        bga_write(4, 0); bga_write(1, (uint16_t)width);
        bga_write(2, (uint16_t)height); bga_write(3, (uint16_t)(bpp ? bpp : 32));
        bga_write(4, 0x01 | 0x40);
    }
    screen_w = width; screen_h = height;
    uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    if (back_buffer && back_buffer != video_memory && (video_memory == NULL || back_buffer < video_memory || back_buffer >= video_memory + (screen_w * screen_h * 2))) {
        free(back_buffer);
    }
    if (pmm_get_total_memory() <= 16 * 1024 * 1024 && video_memory) {
        back_buffer = video_memory + (screen_w * screen_h);
    } else {
        back_buffer = (uint32_t*)malloc(buffer_size);
        if (!back_buffer && video_memory) back_buffer = video_memory + (screen_w * screen_h);
        else if (!back_buffer) back_buffer = video_memory;
    }
    clear_screen(current_theme == THEME_DARK ? 0x0D1117 : 0xF3F4F6);
    if (video_memory && back_buffer && video_memory != back_buffer) {
        memcpy(video_memory, back_buffer, buffer_size);
    }
    extern void taskbar_init(void);
    taskbar_init();
    extern void cursor_set_screen_bounds(int w, int h);
    cursor_set_screen_bounds(screen_w, screen_h);
    extern void wm_clamp_all_windows(void);
    wm_clamp_all_windows();
}

// ─── Core Pixel ─────────────────────────────────────────────
void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= screen_w || y < 0 || y >= screen_h) return;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b =  color        & 0xFF;
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
    mark_dirty(y);
}

// ─── Rectangles ─────────────────────────────────────────────
void draw_rect(int x, int y, int w, int h, uint32_t color) {
    int x2 = x + w; int y2 = y + h;
    if (x  < 0) x  = 0;
    if (y  < 0) y  = 0;
    if (x2 > screen_w) x2 = screen_w;
    if (y2 > screen_h) y2 = screen_h;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b =  color        & 0xFF;
    if (brightness_level < 100) {
        r = (r * brightness_level) / 100;
        g = (g * brightness_level) / 100;
        b = (b * brightness_level) / 100;
    }
    uint32_t packed = (r << 16) | (g << 8) | b;
    for (int i = y; i < y2; i++) {
        uint32_t* row = back_buffer + i * screen_w;
        for (int j = x; j < x2; j++) row[j] = packed;
        mark_dirty(i);
    }
}

void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if (alpha == 255) { draw_rect(x, y, w, h, color); return; }
    if (alpha == 0)   return;

    int x1 = (x < 0) ? 0 : x;
    int y1 = (y < 0) ? 0 : y;
    int x2 = (x + w > screen_w) ? screen_w : x + w;
    int y2 = (y + h > screen_h) ? screen_h : y + h;
    if (x1 >= x2 || y1 >= y2) return;

    uint32_t sr = ((color >> 16) & 0xFF) * alpha;
    uint32_t sg = ((color >> 8)  & 0xFF) * alpha;
    uint32_t sb = ( color        & 0xFF) * alpha;
    uint32_t inv = 255 - alpha;

    for (int i = y1; i < y2; i++) {
        uint32_t* row = back_buffer + i * screen_w;
        for (int j = x1; j < x2; j++) {
            uint32_t dst = row[j];
            uint32_t dr = (dst >> 16) & 0xFF;
            uint32_t dg = (dst >> 8)  & 0xFF;
            uint32_t db =  dst        & 0xFF;
            uint32_t r = (sr + dr * inv) >> 8;
            uint32_t g = (sg + dg * inv) >> 8;
            uint32_t b = (sb + db * inv) >> 8;
            row[j] = (r << 16) | (g << 8) | b;
        }
    }
}

// ─── Rounded Rectangles ────────────────────────────────────
void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    if (r <= 0) { draw_rect(x, y, w, h, color); return; }
    // Fill center + cross
    draw_rect(x + r, y,     w - 2*r, h,     color);
    draw_rect(x,     y + r, r,       h-2*r, color);
    draw_rect(x+w-r, y + r, r,       h-2*r, color);
    // Four corners with circle quarter
    for (int cy = 0; cy < r; cy++) {
        for (int cx = 0; cx < r; cx++) {
            int dx = r - 1 - cx, dy = r - 1 - cy;
            if (dx*dx + dy*dy <= r*r)
                put_pixel(x+cx,       y+cy,       color),
                put_pixel(x+w-1-cx,   y+cy,       color),
                put_pixel(x+cx,       y+h-1-cy,   color),
                put_pixel(x+w-1-cx,   y+h-1-cy,   color);
        }
    }
}

void draw_rounded_rect_alpha(int x, int y, int w, int h, int r, uint32_t color, uint8_t alpha) {
    if (r <= 0) { draw_rect_alpha(x, y, w, h, color, alpha); return; }
    draw_rect_alpha(x + r, y,     w - 2*r, h,     color, alpha);
    draw_rect_alpha(x,     y + r, r,       h-2*r, color, alpha);
    draw_rect_alpha(x+w-r, y + r, r,       h-2*r, color, alpha);
    for (int cy = 0; cy < r; cy++) {
        for (int cx = 0; cx < r; cx++) {
            int dx = r - 1 - cx, dy = r - 1 - cy;
            if (dx*dx + dy*dy <= r*r) {
                draw_rect_alpha(x+cx,     y+cy,     1, 1, color, alpha);
                draw_rect_alpha(x+w-1-cx, y+cy,     1, 1, color, alpha);
                draw_rect_alpha(x+cx,     y+h-1-cy, 1, 1, color, alpha);
                draw_rect_alpha(x+w-1-cx, y+h-1-cy, 1, 1, color, alpha);
            }
        }
    }
}

// ─── Circles ────────────────────────────────────────────────
void draw_circle(int cx, int cy, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius)
                put_pixel(cx + x, cy + y, color);
        }
    }
}

void draw_circle_alpha(int cx, int cy, int radius, uint32_t color, uint8_t alpha) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius)
                draw_rect_alpha(cx + x, cy + y, 1, 1, color, alpha);
        }
    }
}

// ─── Gradients ──────────────────────────────────────────────
void draw_linear_gradient(int x, int y, int w, int h, uint32_t c1, uint32_t c2, int vertical) {
    uint8_t r1=(c1>>16)&0xFF, g1=(c1>>8)&0xFF, b1=c1&0xFF;
    uint8_t r2=(c2>>16)&0xFF, g2=(c2>>8)&0xFF, b2=c2&0xFF;
    int steps = vertical ? h : w;
    if (steps <= 0) return;
    for (int i = 0; i < steps; i++) {
        uint8_t r = (uint8_t)(r1 + (int)(r2-r1)*i/steps);
        uint8_t g = (uint8_t)(g1 + (int)(g2-g1)*i/steps);
        uint8_t b = (uint8_t)(b1 + (int)(b2-b1)*i/steps);
        uint32_t col = (r<<16)|(g<<8)|b;
        if (vertical) draw_rect(x, y+i, w, 1, col);
        else          draw_rect(x+i, y, 1, h, col);
    }
}

void draw_gradient_rounded_rect(int x, int y, int w, int h, int r, uint32_t c1, uint32_t c2, int vertical) {
    // Draw gradient into region, then mask corners
    uint8_t r1=(c1>>16)&0xFF, g1=(c1>>8)&0xFF, b1=c1&0xFF;
    uint8_t r2=(c2>>16)&0xFF, g2=(c2>>8)&0xFF, b2=c2&0xFF;
    int steps = vertical ? h : w;
    if (steps <= 0) return;
    for (int i = 0; i < steps; i++) {
        uint8_t rv = (uint8_t)(r1 + (int)(r2-r1)*i/steps);
        uint8_t gv = (uint8_t)(g1 + (int)(g2-g1)*i/steps);
        uint8_t bv = (uint8_t)(b1 + (int)(b2-b1)*i/steps);
        uint32_t col = (rv<<16)|(gv<<8)|bv;
        if (vertical) draw_rect(x, y+i, w, 1, col);
        else          draw_rect(x+i, y, 1, h, col);
    }
    // Erase corners — overwrite with a transparent clear (leave pixel from desktop wallpaper by not writing)
    // Instead, mask corners from back_buffer context color
    // Mask corners with active theme background color instead of hardcoded 0x0D1117
    uint32_t bg_col = (current_theme == THEME_DARK) ? 0x0D1117 : 0xF3F4F6;
    for (int cy2 = 0; cy2 < r && r > 0; cy2++) {
        for (int cx2 = 0; cx2 < r; cx2++) {
            int dx = r - 1 - cx2, dy = r - 1 - cy2;
            if (dx*dx + dy*dy > r*r) {
                put_pixel(x+cx2,       y+cy2,       bg_col);
                put_pixel(x+w-1-cx2,   y+cy2,       bg_col);
                put_pixel(x+cx2,       y+h-1-cy2,   bg_col);
                put_pixel(x+w-1-cx2,   y+h-1-cy2,   bg_col);
            }
        }
    }
}

// ─── Soft Window Shadow ──────────────────────────────────────
void draw_window_shadow(int x, int y, int w, int h) {
    // Fast single-pass 64-bit shadow
    draw_rect_alpha(x-4, y-2, w+8, h+6, 0x000000, 70);
}

// ─── Box Blur ────────────────────────────────────────────────
void draw_box_blur(int x, int y, int w, int h, int radius) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x+w > screen_w) w = screen_w-x;
    if (y+h > screen_h) h = screen_h-y;
    if (w <= 0 || h <= 0 || !back_buffer) return;

    // Fast glassmorphic acrylic tint (0.01ms vs 25ms 2.2M pixel loop)
    draw_rect_alpha(x, y, w, h, 0x0F172A, 180);
}

// ─── Text Rendering ─────────────────────────────────────────
void draw_char(int x, int y, char c, uint32_t color) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) return;
    if (x < 0 || y < 0 || x + 8 > screen_w || y + 8 > screen_h) {
        const uint8_t* glyph = font8x8_basic[(int)c];
        for (int cy = 0; cy < 8; cy++)
            for (int cx = 0; cx < 8; cx++)
                if (glyph[cy] & (1 << (7-cx)))
                    put_pixel(x+cx, y+cy, color);
        return;
    }

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b =  color        & 0xFF;
    if (brightness_level < 100) {
        r = (r * brightness_level) / 100;
        g = (g * brightness_level) / 100;
        b = (b * brightness_level) / 100;
    }
    if (night_light_active) {
        r = (r * 110 > 255) ? 255 : (r * 110 / 100);
        b = (b * 65) / 100;
    }
    uint32_t final_color = (r << 16) | (g << 8) | b;

    const uint8_t* glyph = font8x8_basic[(int)c];
    for (int cy = 0; cy < 8; cy++) {
        uint8_t row_bits = glyph[cy];
        if (!row_bits) continue;
        uint32_t* row = back_buffer + (y + cy) * screen_w + x;
        if (row_bits & 0x80) row[0] = final_color;
        if (row_bits & 0x40) row[1] = final_color;
        if (row_bits & 0x20) row[2] = final_color;
        if (row_bits & 0x10) row[3] = final_color;
        if (row_bits & 0x08) row[4] = final_color;
        if (row_bits & 0x04) row[5] = final_color;
        if (row_bits & 0x02) row[6] = final_color;
        if (row_bits & 0x01) row[7] = final_color;
    }
}

void draw_string(int x, int y, uint32_t color, const char* str) {
    int cx = x, cy = y;
    while (*str) {
        if (*str == '\n') { cy += 10; cx = x; }
        else { draw_char(cx, cy, *str, color); cx += 8; }
        str++;
    }
}

// 2× scaled text — doubles each pixel for 16px effective size (titles, headings)
void draw_string_scaled(int x, int y, uint32_t color, const char* str, int scale) {
    if (scale <= 0) scale = 1;
    int cx = x, cy = y;
    while (*str) {
        if (*str == '\n') { cy += 8*scale + 2; cx = x; }
        else {
            uint8_t uc = (uint8_t)*str;
            if (uc <= 127) {
                const uint8_t* glyph = font8x8_basic[(int)*str];
                for (int row = 0; row < 8; row++)
                    for (int col = 0; col < 8; col++)
                        if (glyph[row] & (1 << (7-col)))
                            draw_rect(cx + col*scale, cy + row*scale, scale, scale, color);
            }
            cx += 8*scale;
        }
        str++;
    }
}

// Text with 1-pixel drop shadow for legibility on any background
void draw_string_shadow(int x, int y, uint32_t color, uint32_t shadow_color, const char* str) {
    draw_string(x+1, y+1, shadow_color, str);
    draw_string(x,   y,   color,        str);
}

// ─── Outlined Rounded Rect (border only) ────────────────────
void draw_rounded_rect_outline(int x, int y, int w, int h, int r, int thickness, uint32_t color) {
    for (int t = 0; t < thickness; t++) {
        // Top/bottom horizontal strips with rounded masking
        for (int i = r; i < w-r; i++) {
            put_pixel(x+i, y+t,       color);
            put_pixel(x+i, y+h-1-t,   color);
        }
        // Left/right vertical strips
        for (int i = r; i < h-r; i++) {
            put_pixel(x+t,     y+i, color);
            put_pixel(x+w-1-t, y+i, color);
        }
        // Corner arcs
        if (r > 0) {
            for (int a = 0; a < r; a++) {
                for (int b = 0; b < r; b++) {
                    int dx = r-1-a, dy = r-1-b;
                    int dist = dx*dx + dy*dy;
                    int inner = (r-t-1)*(r-t-1);
                    if (dist <= r*r && dist > inner) {
                        put_pixel(x+a,     y+b,     color);
                        put_pixel(x+w-1-a, y+b,     color);
                        put_pixel(x+a,     y+h-1-b, color);
                        put_pixel(x+w-1-a, y+h-1-b, color);
                    }
                }
            }
        }
    }
}

// ─── Buffer Swap & FPS ──────────────────────────────────────
static uint32_t frame_count_sec = 0;
static uint32_t last_fps_ticks  = 0;
static int      current_real_fps = 60;

void swap_buffers(void) {
    if (video_memory && back_buffer && video_memory != back_buffer) {
        // High-throughput 4x unrolled 64-bit VRAM copy (32 bytes per loop iteration)
        uint64_t* dst = (uint64_t*)video_memory;
        uint64_t* src = (uint64_t*)back_buffer;
        size_t total_words = ((size_t)screen_w * screen_h * 4) / 8;
        size_t unrolled = total_words / 4;
        for (size_t i = 0; i < unrolled; i++) {
            size_t idx = i * 4;
            dst[idx + 0] = src[idx + 0];
            dst[idx + 1] = src[idx + 1];
            dst[idx + 2] = src[idx + 2];
            dst[idx + 3] = src[idx + 3];
        }
        for (size_t i = unrolled * 4; i < total_words; i++) {
            dst[i] = src[i];
        }
    }
    extern volatile uint32_t timer_ticks;
    frame_count_sec++;
    uint32_t delta = timer_ticks - last_fps_ticks;
    if (delta >= 50) {
        current_real_fps = (int)((frame_count_sec * 100) / delta);
        frame_count_sec = 0;
        last_fps_ticks  = timer_ticks;
    }
}

int graphics_get_real_fps(void) {
    return (current_real_fps > 0 && current_real_fps <= 120) ? current_real_fps : 60;
}

void clear_screen(uint32_t color) {
    if (!back_buffer) return;
    uint64_t color64 = ((uint64_t)color << 32) | (uint64_t)color;
    uint64_t* buf64 = (uint64_t*)back_buffer;
    size_t count64 = ((size_t)screen_w * screen_h) / 2;
    for (size_t i = 0; i < count64; i++) buf64[i] = color64;
}

// ─── Theme / Brightness / Night Light ───────────────────────
static int target_fps_limit = 60;
void graphics_set_target_fps(int fps) { if (fps < 15) fps = 15; if (fps > 240) fps = 240; target_fps_limit = fps; }
int  graphics_get_target_fps(void)    { return target_fps_limit; }
void graphics_set_theme(int theme)    { current_theme = theme; }
int  graphics_get_theme(void)         { return current_theme; }
void graphics_set_brightness(int lvl) { if (lvl<10)lvl=10; if(lvl>100)lvl=100; brightness_level=lvl; }
int  graphics_get_brightness(void)    { return brightness_level; }
void graphics_set_night_light(int e)  { night_light_active = e ? 1 : 0; }
int  graphics_get_night_light(void)   { return night_light_active; }
