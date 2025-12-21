#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"
#include "graphics.h"
#include "pmm.h"
#include "vmm.h"
#include "idt.h"
#include "io.h"
#include "gdt.h"
#include "vga.h"
#include "usb.h"
#include "timer.h"
#include "heap.h"
#include "string.h"
#include "common.h"
#include "sysinfo.h"

extern char terminal_buffer[];
extern int term_idx;
extern int mouse_x, mouse_y;
extern void init_mouse();

// --- MAIN KERNEL ---
void kmain(void* mb_info_ptr) {
    struct multiboot_info* mb = (struct multiboot_info*)mb_info_ptr;
    
    // EMERGENCY FALLBACK: Use VGA text mode to show we're alive
    // This always works, even if graphics fail
    vga_clear();
    vga_print("CimpleOS Booting...\n");
    vga_print("Initializing GDT...\n");
    
    // Capture Multiboot Info BEFORE enabling paging!
    // (mb pointer might become invalid if it's outside identity mapped region)
    uint32_t mb_flags = mb->flags;
    uint64_t mb_fb_addr = mb->framebuffer_addr;
    uint32_t mb_fb_width = mb->framebuffer_width;
    uint32_t mb_fb_height = mb->framebuffer_height;

    // 1. Setup GDT
    gdt_install();

    // 2. Setup Memory Management
    // We need to initialize PMM first to know what's free.
    pmm_init(mb);
    
    // FIXED VMM: Now uses pre-allocated page tables and maps high memory!
    // This identity maps the first 256MB + framebuffer region
    // No dynamic allocation after paging is enabled = no crashes!
    vga_print("Enabling paging...\n");
    vmm_init();
    vga_print("Paging enabled!\n");

    // 3. Setup Graphics
    // Try to use multiboot framebuffer info, with fallback to common values
    if (mb_fb_addr != 0) {
        video_memory = (uint32_t*) (uint32_t) mb_fb_addr;
        screen_w = (int) mb_fb_width;
        screen_h = (int) mb_fb_height;
    } else {
        // Fallback: Use VGA linear framebuffer at realistic low address
        // 0xFD000000 is typical for some configs, but let's use 0x0FC00000 (252MB) to be safe
        // This ensures it's within the 256MB we'd map if VMM were enabled
        video_memory = (uint32_t*) 0x0FC00000; // 252MB - within 256MB if VMM enabled
        screen_w = 1024;
        screen_h = 768;
    }
    
    clear_screen(0x000000); // Black background
    
    // Welcome Message
    draw_string(10, 10, 0x00FF00, "CimpleOS v0.3 - Protected Mode + Paging Enabled!");
    draw_string(10, 30, 0xFFFFFF, "Memory Management: PMM + VMM Active");
    draw_string(10, 50, 0xFFFFFF, "Graphics: Initialized");

    // 4. Initialize Interrupts
    vga_print("Initializing Interrupts...\n");
    init_idt();
    init_mouse();
    
    // 5. Initialize Timer (100 Hz)
    timer_init(100);
    
    // 6. Initialize Heap
    heap_init();
    
    // 7. Initialize System Info
    sysinfo_init();
    
    asm volatile("sti"); 
    vga_print("Interrupts Enabled!\n");
    
    // 8. Initialize USB (if available)
    vga_print("Checking for USB...\n");
    usb_init();
    
    vga_print("Press any key or move mouse...\n");
    
    // Debug vars
    extern int irq_count;
    int last_count = 0;

    while (1) {
        // Update VGA with interrupt count if it changed
        if (irq_count != last_count) {
            vga_print("INT! ");
            last_count = irq_count;
        }
        // Simple Shell UI
        draw_rect(0, 0, screen_w, screen_h, 0x111111); // Background
        
        // Top Bar
        draw_rect(0, 0, screen_w, 20, 0x333333);
        draw_string(5, 5, 0xFFFFFF, "CimpleOS");

        // Terminal Window
        draw_rect(10, 100, 600, 400, 0x000000);
        draw_rect(10, 100, 600, 20, 0xCCCCCC); // Title bar
        draw_string(15, 105, 0x000000, "Terminal");
        
        // Draw Terminal Content
        draw_string(15, 130, 0x00FF00, "$ ");
        draw_string(35, 130, 0xFFFFFF, terminal_buffer);
        
        // Cursor
        draw_rect(35 + (term_idx * 8), 130, 8, 8, 0xFFFFFF);

        // Mouse
        draw_rect(mouse_x, mouse_y, 5, 5, 0xFF0000); 

        // Debug Info
        // We need a simple itoa here or just print hex manually? 
        // For now, let's just assume we can see if it changes.
        // (I will implement a quick hex printer in graphics.c if needed, but for now let's rely on visual feedback)
        // Actually, let's just draw a pixel counter at the bottom
        draw_rect(10 + (irq_count % 100), 600, 5, 5, 0x00FF00); // Moving dot = interrupts working

        swap_buffers();
    }
}