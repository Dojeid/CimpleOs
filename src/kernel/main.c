#include <stdint.h>
#include <stddef.h>
// Include headers
#include "include/multiboot.h"
#include "include/common.h"
// Drivers
#include "drivers/video/graphics.h"
#include "drivers/video/vga.h"
#include "drivers/bus/pci.h"
#include "drivers/bus/usb.h"
#include "drivers/input/mouse.h"
#include "drivers/input/keyboard.h"
// Memory management
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
// Library
#include "lib/io.h"
#include "lib/string.h"
// Kernel
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "kernel/sysinfo.h"
#include "kernel/cmd.h"
// GUI
#include "gui/terminal.h"
#include "gui/window_manager.h"
#include "gui/desktop.h"
#include "gui/taskbar.h"
#include "gui/cursor.h"

extern char terminal_buffer[];
extern int term_idx;
extern int mouse_x, mouse_y;
extern void init_mouse();

// --- MAIN KERNEL ---
void kmain(void* multiboot_info_addr) {
    multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info_addr;
    
    // EMERGENCY FALLBACK: Use VGA text mode to show we're alive
    // This always works, even if graphics fail
    vga_clear();
    vga_print("CimpleOS Booting...\n");
    vga_print("Initializing GDT...\n");
    
    // Capture Multiboot Info BEFORE enabling paging!
    // (mb pointer might become invalid if it's outside identity mapped region)
    uint32_t mb_flags = mbi->flags;
    uint64_t mb_fb_addr = mbi->framebuffer_addr;
    uint32_t mb_fb_width = mbi->framebuffer_width;
    uint32_t mb_fb_height = mbi->framebuffer_height;

    // 1. Setup GDT
    gdt_install();

    // 2. Setup Memory Management
    // We need to initialize PMM first to know what's free.
    pmm_init(mbi);
    
    // FIXED VMM: Now uses pre-allocated page tables and maps high memory!
    // This identity maps the first 256MB + framebuffer region
    // No dynamic allocation after paging is enabled = no crashes!
    vga_print("Enabling paging...\n");
    vmm_init();
    vga_print("Paging enabled!\n");

    // 3. Setup Graphics
    vga_print("Initializing Graphics...\n");
    graphics_init(mb);  // THIS WAS MISSING! Sets up video_memory, screen size, and back_buffer
    
    clear_screen(0x000000); // Black background
    
    // Welcome Message
    draw_string(10, 10, 0x00FF00, "CimpleOS v0.4 - Protected Mode + Paging Enabled!");
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
    
    // asm volatile("sti"); // Interrupts enabled later after full hardware setup
    // vga_print("Interrupts Enabled!\n");
    
    // 7. Initialize USB (if available)
    vga_print("Checking for USB...\n");
    usb_init();
    
    vga_print("System ready! Starting GUI...\n");
    
    // Initialize hardware
    gdt_install();
    init_idt();  // Now 64-bit compatible
    timer_init(100);
    desktop_init();
    taskbar_init();
    cursor_init();
    
    // Enable interrupts after all critical hardware is set up
    asm volatile("sti");
    vga_print("Interrupts Enabled!\n");

    // Initialize GUI subsystems
    terminal_init();
    wm_init();
    
    // Create terminal window
    window_t* term_win = wm_create_window(50, 80, 700, 480, "Terminal");
    if (term_win) {
        // FEATURE 1: Create independent terminal instance
        term_win->user_data = terminal_create_instance();
        taskbar_add_button(term_win->id, "Terminal");
        term_win->render_content = NULL;
        
        // Print welcome to this instance (or global if malloc failed)
        terminal_instance_t* term = (terminal_instance_t*)term_win->user_data;
        if (term) {
            terminal_instance_print(term, "Welcome to CimpleOS v0.4 GUI!");
            terminal_instance_print(term, "Windowing system active.");
            terminal_instance_print(term, "");
            terminal_instance_print(term, "Type 'help' for available commands.");
            terminal_instance_print(term, "Use UP/DOWN arrows for history.");
            terminal_instance_print(term, "Drag windows by title bar!");
            terminal_instance_print(term, "Click green 'Terminal' button for more terminals.");
            terminal_instance_print(term, "");
        } else {
            // Fallback to global terminal
            terminal_print("Welcome to CimpleOS v0.4 GUI!");
            terminal_print("Windowing system active.");
            terminal_print("");
            terminal_print("Type 'help' for available commands.");
            terminal_print("WARNING: Terminal instance creation failed - using shared global terminal");
            terminal_print("");
        }
    }
    
    // Mouse state for click detection
    int last_mouse_btn = 0;  // Moved outside loop for clarity

    while (1) {
        // Handle mouse interactions
        int mouse_btn = mouse_button_left();
        
        // Mouse button pressed
        if (mouse_btn && !last_mouse_btn) {
            // Check taskbar first
            if (mouse_y >= screen_h - 30) {
                taskbar_handle_click(mouse_x, mouse_y);
            } else {
                wm_handle_mouse_down(mouse_x, mouse_y);
            }
        }
        
        // Mouse button released
        if (!mouse_btn && last_mouse_btn) {
            wm_handle_mouse_up(mouse_x, mouse_y);
        }
        
        // Mouse dragging
        if (mouse_btn) {
            wm_handle_mouse_move(mouse_x, mouse_y);
        }
        
        last_mouse_btn = mouse_btn;
        
        // Update cursor position
        cursor_set_position(mouse_x, mouse_y);
        
        // === RENDER EVERYTHING ===
        
        // 1. Desktop background
        desktop_render_background();
        desktop_render_topbar();
        
        // 2. FEATURE 1: Render ALL terminal windows (not just first one!)
        window_manager_t* wm_state = wm_get_state();
        for (int i = 0; i < MAX_WINDOWS; i++) {
            window_t* win = &wm_state->windows[i];
            
            // Skip invalid or minimized windows
            if (win->id == -1) continue;
            if (!(win->flags & WIN_FLAG_VISIBLE)) continue;
            if (win->flags & WIN_FLAG_MINIMIZED) continue;
            
            // Check if this window has a terminal instance
            terminal_instance_t* term = (terminal_instance_t*)win->user_data;
            if (!term) continue;  // Not a terminal window
            
            // Render this terminal's content
            int win_content_x = win->x;
            int win_content_y = win->y + TITLEBAR_HEIGHT;
            int win_content_h = win->height;
            
            // Terminal output area
            terminal_instance_render(term, win_content_x + 10, win_content_y + 10);
            
            // Input line at bottom of window (only for focused terminal)
            if (win->id == wm_state->focused_window_id) {
                int input_y = win_content_y + win_content_h - 25;
                draw_string(win_content_x + 10, input_y, 0x00FF00, "$ ");
                draw_string(win_content_x + 30, input_y, 0xFFFFFF, terminal_buffer);
                
                // Cursor blink
                extern volatile int irq_count;
                if ((irq_count / 25) % 2 == 0) {
                    draw_rect(win_content_x + 30 + (term_idx * 8), input_y, 8, 12, 0xFFFFFF);
                }
            }
        }
        
        // Render window frames (title bars, buttons, borders)
        wm_render_all();
        
        // 3. Taskbar (always on top)
        taskbar_render();
        
        // 4. Cursor (absolutely last - on top of everything)
        cursor_render();
        
        // Swap buffers to display
        swap_buffers();
    }
}