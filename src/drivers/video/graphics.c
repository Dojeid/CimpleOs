#include "graphics.h"
#include "include/font.h"
#include "lib/string.h"
#include "lib/io.h"
#include "mm/heap.h"
#include "drivers/bus/pci.h"
#include "gui/taskbar.h"
#include <stddef.h>

#define MAX_SCREEN_W 1920
#define MAX_SCREEN_H 1080
#define MAX_BUFFER_SIZE (MAX_SCREEN_W * MAX_SCREEN_H * sizeof(uint32_t))

uint32_t* video_memory = NULL;
int screen_w = 1024;
int screen_h = 768;
uint32_t* back_buffer = NULL;

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
            bga_write(4, 0);           // VBE_DISPI_DISABLED
            bga_write(1, 1024);        // Width
            bga_write(2, 768);         // Height
            bga_write(3, 32);          // 32 bpp
            bga_write(5, 1024);        // Virt Width
            bga_write(6, 0);           // X offset
            bga_write(7, 0);           // Y offset
            bga_write(4, 0x01 | 0x40); // VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
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
    
    // Allocate back_buffer for MAX_BUFFER_SIZE (1920x1080) to prevent resolution change crashes
    back_buffer = (uint32_t*)kmalloc(MAX_BUFFER_SIZE);
    if (back_buffer) {
        memset(back_buffer, 0, MAX_BUFFER_SIZE);
    } else {
        back_buffer = video_memory;
    }

    if (!video_memory) {
        video_memory = back_buffer;
    }
}

void put_pixel(int x, int y, uint32_t color) {
    if (!back_buffer) return;
    if (x < 0 || y < 0) return;
    if (x >= screen_w || y >= screen_h) return;
    
    back_buffer[(size_t)y * (size_t)screen_w + (size_t)x] = color;
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

static int sys_brightness = 100;
static int sys_night_light = 0;
static int sys_fps_preset = 2; // 1=30, 2=60, 3=90, 4=120, 5=Uncapped

void graphics_set_brightness(int percent) {
    if (percent < 20) percent = 20;
    if (percent > 100) percent = 100;
    sys_brightness = percent;
}

int graphics_get_brightness(void) {
    return sys_brightness;
}

void graphics_set_night_light(int enabled) {
    sys_night_light = enabled ? 1 : 0;
}

int graphics_get_night_light(void) {
    return sys_night_light;
}

void graphics_set_fps_target(int fps_preset) {
    if (fps_preset < 1) fps_preset = 1;
    if (fps_preset > 5) fps_preset = 5;
    sys_fps_preset = fps_preset;
}

int graphics_get_fps_delay_ticks(void) {
    switch (sys_fps_preset) {
        case 1: return 3; // 30 FPS
        case 2: return 1; // 60 FPS
        case 3: return 1; // 90 FPS
        case 4: return 1; // 120 FPS
        case 5: default: return 0; // Uncapped
    }
}

void graphics_set_resolution(int width, int height) {
    if (width < 640 || height < 480) return;
    if (width > MAX_SCREEN_W) width = MAX_SCREEN_W;
    if (height > MAX_SCREEN_H) height = MAX_SCREEN_H;

    screen_w = width;
    screen_h = height;
    
    // Program Bochs BGA Hardware Registers
    uint16_t bga_id = bga_read(0);
    if (bga_id >= 0xB0C0 && bga_id <= 0xB0C6) {
        bga_write(4, 0);               // VBE_DISPI_DISABLED
        bga_write(1, (uint16_t)width);  // Width
        bga_write(2, (uint16_t)height); // Height
        bga_write(3, 32);              // 32 bpp
        bga_write(5, (uint16_t)width);  // Virt Width
        bga_write(6, 0);               // X offset
        bga_write(7, 0);               // Y offset
        bga_write(4, 0x01 | 0x40);     // VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
    }

    if (back_buffer) {
        memset(back_buffer, 0, (size_t)width * (size_t)height * sizeof(uint32_t));
    }
    
    taskbar_init();
}

void swap_buffers() {
    if (!video_memory || !back_buffer) return;

    size_t copy_bytes = (size_t)screen_w * (size_t)screen_h * sizeof(uint32_t);

    if (sys_brightness == 100 && !sys_night_light) {
        memcpy(video_memory, back_buffer, copy_bytes);
    } else {
        int total = screen_w * screen_h;
        for (int i = 0; i < total; i++) {
            uint32_t pixel = back_buffer[i];
            uint32_t r = (pixel >> 16) & 0xFF;
            uint32_t g = (pixel >> 8) & 0xFF;
            uint32_t b_ch = pixel & 0xFF;
            
            r = (r * sys_brightness) / 100;
            g = (g * sys_brightness) / 100;
            b_ch = (b_ch * sys_brightness) / 100;
            
            if (sys_night_light) {
                b_ch = (b_ch * 65) / 100;
            }
            
            video_memory[i] = (r << 16) | (g << 8) | b_ch;
        }
    }
}

void clear_screen(uint32_t color) {
    if (!back_buffer) return;
    int total = screen_w * screen_h;
    for(int i=0; i<total; i++) back_buffer[i] = color;
}
