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
#include "drivers/audio/sound.h"
#include "drivers/storage/ata.h"
#include "drivers/input/keyboard.h"
#include "drivers/input/mouse.h"
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
#include "drivers/storage/ata.h"
// FS / Process / Syscall
#include "fs/vfs.h"
#include "fs/ramdisk.h"
#include "fs/ext4.h"
#include "kernel/process.h"
#include "kernel/syscall.h"
// GUI
#include "gui/terminal.h"
#include "gui/window_manager.h"
#include "gui/desktop.h"
#include "gui/taskbar.h"
#include "gui/cursor.h"
#include "gui/apps/file_explorer.h"
#include "gui/apps/notepad.h"
#include "gui/apps/sysmon.h"
#include "lib/printf.h"

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
    vga_print("Falkon-OS Booting...\n");
    vga_print("Initializing GDT...\n");

    // 1. Setup GDT
    gdt_install();

    // 2. Setup Memory Management
    // The custom bootloader does not provide a Multiboot info structure (EBX=0),
    // so fall back to a sane default when it is missing or lacks the memory flag.
    uint64_t total_mem_bytes = 0;
    if (mbi && (mbi->flags & 0x1)) {
        total_mem_bytes = (uint64_t)(mbi->mem_upper + mbi->mem_lower) * 1024;
    }
    pmm_init(total_mem_bytes > 0 ? total_mem_bytes : 128 * 1024 * 1024);

    vga_print("Enabling paging...\n");
    vmm_init();
    vga_print("Paging enabled!\n");

    // Heap must be available before graphics_init allocates the back buffer.
    heap_init();

    // 3. Setup Graphics
    vga_print("Initializing Graphics...\n");
    graphics_init(mbi);

    clear_screen(0x000000); // Black background

    // Welcome Message
    draw_string(10, 10, 0x00FF00, "Falkon-OS v0.4 - Protected Mode + Paging Enabled!");
    draw_string(10, 30, 0xFFFFFF, "Memory Management: PMM + VMM Active");
    draw_string(10, 50, 0xFFFFFF, "Graphics: Initialized");

    // 4. Initialize Interrupts & Hardware
    vga_print("Initializing Interrupts & Peripherals...\n");
    init_idt();
    init_mouse();
    timer_init(100);
    sysinfo_init();
    
    // 5. Initialize USB, Audio & ATA Storage
    vga_print("Checking for USB, Audio & ATA Storage...\n");
    usb_init();
    sound_init();
    ata_init();
    
    vga_print("System ready! Starting GUI...\n");
    
    desktop_init();
    taskbar_init();
    cursor_init();
    
    // Enable interrupts after all critical hardware is set up
    asm volatile("sti");
    vga_print("Interrupts Enabled!\n");

    // Initialize VFS, Ramdisk, Process Scheduler, and Syscalls
    vfs_init();
    ext4_init();
    
    // Attempt to mount EXT4 volume to root
    if (vfs_mount("hda", "/", "ext4") != 0) {
        vga_print("[VFS] Failed to mount EXT4 root filesystem. Ensure volume is formatted.\n");
    }

    ramdisk_init();
    process_init();
    syscall_init();

    // Create system background processes
    process_create("gui_compositor", 0);
    process_create("input_poller", 0);

    // Initialize GUI subsystems
    terminal_init();
    wm_init();
    
    // Create default Falkon Bash Terminal window
    window_t* term_win = wm_create_window(50, 80, 680, 440, "Falkon Bash (fbash)");
    if (term_win) {
        term_win->user_data = terminal_get_state();
        taskbar_add_button(term_win->id, "Falkon Bash");
        term_win->render_content = NULL;
        
        terminal_instance_t* term = (terminal_instance_t*)term_win->user_data;
        if (term) {
            terminal_instance_print(term, "Falkon Bash (fbash) v1.0 POSIX Interactive Shell");
            terminal_instance_print(term, "=================================================");
            terminal_instance_print(term, "Ring 3 ELF Loader, VFS, Ramdisk & Syscall ABI Active.");
            terminal_instance_print(term, "Type 'help' for command list.");
            terminal_instance_print(term, "Type 'vlc /videos/sample.mp4', 'ls', 'ps', or 'fetch'.");
            terminal_instance_print(term, "");
        }
    }

    // Mouse state for click detection
    int last_mouse_btn = 0;  // Moved outside loop for clarity

    while (1) {
        // Poll VirtualBox absolute mouse integration
        mouse_update_vbox();

        // Handle mouse interactions
        int mouse_btn = mouse_button_left();
        
        // Mouse button pressed
        if (mouse_btn && !last_mouse_btn) {
            taskbar_t* tb = taskbar_get_state();
            int menu_y = screen_h - 30 - 240;
            if (mouse_y >= screen_h - 30 || (tb->start_menu_open && mouse_y >= menu_y && mouse_x <= 200)) {
                taskbar_handle_click(mouse_x, mouse_y);
            } else {
                int clicked_win = wm_get_window_at(mouse_x, mouse_y);
                if (clicked_win != -1) {
                    wm_handle_mouse_down(mouse_x, mouse_y);
                } else {
                    desktop_handle_click(mouse_x, mouse_y);
                }
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
            
            // Terminal windows are the ones WITHOUT a render callback
            // (app windows use render_content and store their own user_data).
            terminal_instance_t* term = NULL;
            if (win->render_content == NULL) {
                term = (terminal_instance_t*)win->user_data;
            }
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
                char prompt[128];
                sprintf(prompt, "root@falkon:%s$ ", term->cwd[0] ? term->cwd : "/");
                draw_string(win_content_x + 10, input_y, 0x00FF00, prompt);
                int prompt_w = strlen(prompt) * 8;
                draw_string(win_content_x + 10 + prompt_w, input_y, 0xFFFFFF, terminal_buffer);
                
                // Cursor blink (driven by PIT timer ticks, 100 Hz)
                extern volatile uint32_t timer_ticks;
                if ((timer_ticks / 25) % 2 == 0) {
                    draw_rect(win_content_x + 10 + prompt_w + (term_idx * 8), input_y, 8, 12, 0xFFFFFF);
                }
            }
        }
        
        // Render window frames (title bars, buttons, borders)
        wm_render_all();
        
        // 3. Taskbar (always on top)
        taskbar_render();
        
        // 4. Cursor (absolutely last - on top of everything)
        cursor_render();
        
        // Swap buffers to display (applies brightness & night light tint)
        swap_buffers();

        // Frame rate target delay pacing
        timer_wait(1);
    }
}