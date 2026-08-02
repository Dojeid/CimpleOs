// ============================================================
// graphics.c — Falkon-OS Premium GPU Rendering Engine v2.0
// Full Windows 11 visual fidelity primitives
// ============================================================
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
static int brightness_level = 100;
static int night_light_active = 0;

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

    // BUG FIX #26: Bit 12 (0x1000) indicates Multiboot framebuffer info present
    if (mb && (mb->flags & 0x1000) && mb->framebuffer_addr != 0) {
        video_memory = (uint32_t*)(uintptr_t)mb->framebuffer_addr;
        screen_w = (int)mb->framebuffer_width;
        screen_h = (int)mb->framebuffer_height;
    }

    if (!video_memory) {
        uint16_t bga_id = bga_read(0);
        if (bga_id >= 0xB0C0 && bga_id <= 0xB0C6) {
            bga_write(4, 0); bga_write(1, 1024); bga_write(2, 768);
            bga_write(3, 32); bga_write(4, 0x01 | 0x40);
        }
        struct pci_device pci_vga;
        if (pci_find_device(0x03, 0x00, 0x00, &pci_vga) || pci_find_device(0x03, 0x80, 0x00, &pci_vga)) {
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

    uint32_t buffer_size = screen_w * screen_h * sizeof(uint32_t);
    back_buffer = (uint32_t*)malloc(buffer_size);
    if (!back_buffer) {
        back_buffer = video_memory;
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
    if (back_buffer && back_buffer != video_memory) free(back_buffer);
    back_buffer = (uint32_t*)malloc(buffer_size);
    if (!back_buffer) back_buffer = video_memory;
    clear_screen(current_theme == THEME_DARK ? 0x0D1117 : 0xF3F4F6);
    if (video_memory && back_buffer && video_memory != back_buffer) {
        memcpy(video_memory, back_buffer, buffer_size);
    }
    extern void taskbar_init(void);
    taskbar_init();
    extern void cursor_set_screen_bounds(int w, int h);
    cursor_set_screen_bounds(screen_w, screen_h);
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
    }
}

void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if (alpha == 255) { draw_rect(x, y, w, h, color); return; }
    if (alpha == 0)   return;
    uint8_t sr = (color >> 16) & 0xFF;
    uint8_t sg = (color >> 8)  & 0xFF;
    uint8_t sb =  color        & 0xFF;
    uint16_t inv = 255 - alpha;
    for (int i = 0; i < h; i++) {
        int py = y + i;
        if (py < 0 || py >= screen_h) continue;
        for (int j = 0; j < w; j++) {
            int px = x + j;
            if (px < 0 || px >= screen_w) continue;
            uint32_t dst = back_buffer[py * screen_w + px];
            uint8_t dr = (dst >> 16) & 0xFF;
            uint8_t dg = (dst >> 8)  & 0xFF;
            uint8_t db =  dst        & 0xFF;
            uint8_t r = (sr * alpha + dr * inv) >> 8;
            uint8_t g = (sg * alpha + dg * inv) >> 8;
            uint8_t b = (sb * alpha + db * inv) >> 8;
            back_buffer[py * screen_w + px] = (r << 16) | (g << 8) | b;
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
    // Multi-pass soft shadow — largest to smallest
    draw_rect_alpha(x-12, y-8,  w+24, h+20, 0x000000, 18);
    draw_rect_alpha(x-8,  y-5,  w+16, h+13, 0x000000, 30);
    draw_rect_alpha(x-5,  y-3,  w+10, h+8,  0x000000, 50);
    draw_rect_alpha(x-2,  y-1,  w+4,  h+4,  0x000000, 80);
    // Subtle colored tint (blue-black depth)
    draw_rect_alpha(x-14, y+h-4, w+28, 18,  0x000020, 40);
}

// ─── Box Blur ────────────────────────────────────────────────
void draw_box_blur(int x, int y, int w, int h, int radius) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x+w > screen_w) w = screen_w-x;
    if (y+h > screen_h) h = screen_h-y;
    if (w <= 0 || h <= 0 || radius <= 0 || !back_buffer) return;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int px = x+j, py = y+i;
            uint32_t rs=0, gs=0, bs=0, cnt=0;
            for (int dy = -radius; dy <= radius; dy++) {
                int sy = py+dy;
                if (sy < 0 || sy >= screen_h) continue;
                for (int dx = -radius; dx <= radius; dx++) {
                    int sx = px+dx;
                    if (sx < 0 || sx >= screen_w) continue;
                    uint32_t c = back_buffer[sy*screen_w+sx];
                    rs += (c>>16)&0xFF; gs += (c>>8)&0xFF; bs += c&0xFF; cnt++;
                }
            }
            if (cnt > 0)
                back_buffer[py*screen_w+px] = ((rs/cnt)<<16)|((gs/cnt)<<8)|(bs/cnt);
        }
    }
}

// ─── Text Rendering ─────────────────────────────────────────
void draw_char(int x, int y, char c, uint32_t color) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) return;
    const uint8_t* glyph = font8x8_basic[(int)c];
    for (int cy = 0; cy < 8; cy++)
        for (int cx = 0; cx < 8; cx++)
            if (glyph[cy] & (1 << (7-cx)))
                put_pixel(x+cx, y+cy, color);
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
        memcpy(video_memory, back_buffer, screen_w * screen_h * 4);
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
    for (int i = 0; i < screen_w * screen_h; i++) back_buffer[i] = color;
}

// ─── Theme / Brightness / Night Light ───────────────────────
void graphics_set_theme(int theme)    { current_theme = theme; }
int  graphics_get_theme(void)         { return current_theme; }
void graphics_set_brightness(int lvl) { if (lvl<10)lvl=10; if(lvl>100)lvl=100; brightness_level=lvl; }
int  graphics_get_brightness(void)    { return brightness_level; }
void graphics_set_night_light(int e)  { night_light_active = e ? 1 : 0; }
int  graphics_get_night_light(void)   { return night_light_active; }
