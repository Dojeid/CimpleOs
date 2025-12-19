#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"
#include "graphics.h"
#include "pmm.h"
#include "vmm.h"
#include "idt.h"
#include "io.h"
#include "gdt.h"

extern char terminal_buffer[];
extern int term_idx;
extern int mouse_x, mouse_y;
extern void init_mouse();

// --- MAIN KERNEL ---
void kmain(void* mb_info_ptr) {
    struct multiboot_info* mb = (struct multiboot_info*)mb_info_ptr;
    
    // 1. Setup GDT
    gdt_install();

    // 2. Setup Memory Management
    // We need to initialize PMM first to know what's free.
    pmm_init(mb);
    
    // Then VMM to enable paging (Memory Safety)
    vmm_init();

    // 3. Setup Graphics
    graphics_init(mb);
    clear_screen(0x000000); // Black background

    // 4. Initialize Interrupts
    init_idt();
    init_mouse();
    
    asm volatile("sti"); 

    // Welcome Message
    draw_string(10, 10, 0x00FF00, "CimpleOS v0.2 - Protected Mode Enabled");
    draw_string(10, 30, 0xFFFFFF, "Memory Management: Paging Enabled (Identity Mapped)");
    
    char mem_str[32] = "Free Memory: Checking...";
    draw_string(10, 50, 0xAAAAAA, mem_str);

    while (1) {
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

        swap_buffers();
    }
}