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
        // Attempt BGA setup for 1024x768 32bpp
        uint16_t bga_id = bga_read(0);
        if (bga_id >= 0xB0C0 && bga_id <= 0xB0C6) {
            bga_write(4, 0);           // VBE_DISPI_DISABLED
            bga_write(1, 1024);        // Width
            bga_write(2, 768);         // Height
            bga_write(3, 32);          // 32 bpp
            bga_write(4, 0x01 | 0x40); // VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
        }

        // Search PCI bus for Display Controller (Class 0x03)
        struct pci_device pci_vga;
        if (pci_find_device(0x03, 0x00, 0x00, &pci_vga) || pci_find_device(0x03, 0x80, 0x00, &pci_vga)) {
            if (pci_vga.bar0 != 0) {
                video_memory = (uint32_t*)(uintptr_t)pci_vga.bar0;
            }
        }

        // Fallback standard physical LFB addresses for QEMU / VirtualBox
        if (!video_memory) {
            uint32_t* candidates[] = {
                (uint32_t*)0xFD000000, // QEMU std vga / virtio LFB
                (uint32_t*)0xE0000000, // VirtualBox VMSVGA / VBoxVGA LFB
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
