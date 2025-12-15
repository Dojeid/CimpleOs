#include <stdint.h>
#include <stddef.h>
#include "idt.h"

// --- 1. GLOBAL VARIABLES (Must be at the top!) ---
uint32_t* video_memory;
int screen_width;
int screen_height;

// --- 2. MULTIBOOT STRUCTURE ---
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
};

// --- 3. IO PORT HELPERS ---
void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// --- 4. GRAPHICS FUNCTIONS ---
// (Defined BEFORE they are used in the keyboard handler)

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        video_memory[y * screen_width + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_window(int x, int y, int w, int h, uint32_t title_color, const char* title) {
    draw_rect(x + 5, y + 5, w, h, 0x00202020); // Shadow
    draw_rect(x, y, w, h, 0x00F0F0F0);         // Body
    draw_rect(x, y, w, 25, title_color);       // Title Bar
    draw_rect(x + w - 22, y + 3, 18, 18, 0x00FF5555); // Close Button
}

// --- 5. KEYBOARD HANDLER ---
char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',    0,  ' ',    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, '-',    0,    0,
    0, '+',   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

// **IMPORTANT**: This function name must match what is in interrupts.asm
void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    if (!(scancode & 0x80)) { // If key pressed
        char letter = kbd_US[scancode];
        if (letter != 0) {
            // Now this works because draw_rect is defined above!
            draw_rect(10, 10, 20, 20, 0xFFFFFF); // Flash a white box
        }
    }
    outb(0x20, 0x20); // Send EOI
}

// --- 6. MAIN KERNEL ENTRY ---
void kmain(struct multiboot_info* mb_info) {
    // 1. Setup Graphics Pointers
    video_memory = (uint32_t*) (uint32_t) mb_info->framebuffer_addr;
    screen_width = (int) mb_info->framebuffer_width;
    screen_height = (int) mb_info->framebuffer_height;

    // 2. Draw UI
    draw_rect(0, 0, screen_width, screen_height, 0x00008080); // Background
    draw_rect(0, screen_height - 40, screen_width, 40, 0x00333333); // Taskbar
    draw_rect(5, screen_height - 35, 80, 30, 0x002ECC71); // Start Button
    
    draw_window(100, 100, 400, 200, 0x003498DB, "Welcome");
    draw_window(150, 150, 400, 200, 0x002C3E50, "Terminal");

    // 3. Enable Interrupts
    init_idt();
    asm volatile("sti");
}