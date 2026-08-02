#include <stdint.h>
#include <stddef.h>
#include "include/multiboot.h"
#include "include/common.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vga.h"
#include "drivers/bus/pci.h"
#include "drivers/bus/usb.h"
#include "drivers/audio/sound.h"
#include "drivers/storage/ata.h"
#include "drivers/input/keyboard.h"
#include "drivers/input/mouse.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "lib/io.h"
#include "lib/string.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "kernel/sysinfo.h"
#include "kernel/cmd.h"
#include "fs/vfs.h"
#include "fs/ramdisk.h"
#include "fs/ext4.h"
#include "kernel/process.h"
#include "kernel/syscall.h"
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

int sys_runlevel = 3; // 3 = Core Linux TTY CLI Mode, 5 = Graphical Desktop (startx)

void sys_set_runlevel(int level) {
    if (level == 3 || level == 5) {
        sys_runlevel = level;
        clear_screen(0x000000);
        if (level == 5) {
            vga_print("Starting Graphical Desktop Environment (startx)...\n");
        } else {
            vga_print("Switched to Core Linux Console TTY Mode (Runlevel 3).\n");
        }
    }
}

int sys_get_runlevel(void) {
    return sys_runlevel;
}

void kmain(void* multiboot_info_addr) {
    multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info_addr;

    vga_clear();
    vga_print("Falkon-OS Booting...\n");
    vga_print("Initializing GDT...\n");

    gdt_install();

    uint64_t total_mem_bytes = 0;
    if (mbi && (mbi->flags & 0x1)) {
        total_mem_bytes = (uint64_t)(mbi->mem_upper + mbi->mem_lower) * 1024;
    }
    pmm_init(total_mem_bytes > 0 ? total_mem_bytes : 128 * 1024 * 1024);

    vga_print("Enabling paging...\n");
    vmm_init();
    vga_print("Paging enabled!\n");

    heap_init();

    vga_print("Initializing Graphics Framebuffer...\n");
    graphics_init(mbi);

    clear_screen(0x000000);

    vga_print("Initializing Interrupts & Peripherals...\n");
    init_idt();
    init_mouse();
    timer_init(100);
    sysinfo_init();
    
    usb_init();
    sound_init();
    ata_init();
    
    vga_print("System ready! Starting Core Linux Services...\n");
    
    desktop_init();
    taskbar_init();
    cursor_init();
    
    asm volatile("sti");
    vga_print("Interrupts Enabled!\n");

    vfs_init();
    ext4_init();
    
    if (vfs_mount("hda", "/", "ext4") != 0) {
        vga_print("[VFS] Mounted ISO/Ramdisk root filesystem.\n");
    }

    ramdisk_init();
    process_init();
    syscall_init();

    process_create("gui_compositor", 0);
    process_create("input_poller", 0);

    terminal_init();
    wm_init();
    
    // Create default Falkon Bash Terminal window for Desktop GUI
    window_t* term_win = wm_create_window(50, 80, 680, 440, "Falkon Bash (fbash)");
    if (term_win) {
        term_win->user_data = terminal_get_state();
        taskbar_add_button(term_win->id, "Falkon Bash");
        term_win->render_content = NULL;
        
        terminal_instance_t* term = (terminal_instance_t*)term_win->user_data;
        if (term) {
            terminal_instance_print(term, "Falkon Bash (fbash) v1.0 POSIX Interactive Shell");
            terminal_instance_print(term, "=================================================");
            terminal_instance_print(term, "Type 'startx' to switch to Desktop GUI window manager.");
            terminal_instance_print(term, "Type 'help' for Linux CLI command list.");
            terminal_instance_print(term, "");
        }
    }

    // Default boot banner in CLI mode
    terminal_instance_t* root_tty = (terminal_instance_t*)terminal_get_state();
    terminal_instance_print(root_tty, "Falkon-OS v1.0 Enterprise (x86_64-falkon-elf)");
    terminal_instance_print(root_tty, "Linux 6.8.0-falkon #1 SMP PREEMPT 2026 x86_64 GNU/Linux");
    terminal_instance_print(root_tty, "");
    terminal_instance_print(root_tty, "falkon-os login: root (automatic login)");
    terminal_instance_print(root_tty, "Type 'startx' to launch Desktop GUI Window Manager.");
    terminal_instance_print(root_tty, "Type 'help' for command list.");
    terminal_instance_print(root_tty, "");

    int last_mouse_btn = 0;

    while (1) {
        // Poll VirtualBox absolute mouse integration
        mouse_update_vbox();

        if (sys_runlevel == 5) {
            // === RUNLEVEL 5: GRAPHICAL DESKTOP GUI MODE ===
            int mouse_btn = mouse_button_left();
            
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
            
            if (!mouse_btn && last_mouse_btn) {
                wm_handle_mouse_up(mouse_x, mouse_y);
            }
            
            if (mouse_btn) {
                wm_handle_mouse_move(mouse_x, mouse_y);
            }
            
            last_mouse_btn = mouse_btn;
            cursor_set_position(mouse_x, mouse_y);
            
            desktop_render_background();
            desktop_render_topbar();
            
            wm_render_all();
            
            // Render terminal content inside windows
            window_manager_t* wm_state = wm_get_state();
            for (int i = 0; i < MAX_WINDOWS; i++) {
                window_t* win = &wm_state->windows[i];
                if (win->id == -1 || !(win->flags & WIN_FLAG_VISIBLE) || (win->flags & WIN_FLAG_MINIMIZED)) continue;
                
                terminal_instance_t* term = NULL;
                if (win->render_content == NULL) {
                    term = (terminal_instance_t*)win->user_data;
                }
                if (!term) continue;
                
                int win_content_x = win->x;
                int win_content_y = win->y + TITLEBAR_HEIGHT;
                terminal_instance_render(term, win_content_x + 10, win_content_y + 10);
            }
            
            taskbar_render();
            cursor_render();
            swap_buffers();
        } else {
            // === RUNLEVEL 3: CORE LINUX CONSOLE TTY CLI MODE ===
            clear_screen(0x0A0E17);
            
            draw_rect(0, 0, screen_w, 24, 0x1E293B);
            draw_string(10, 6, 0x38BDF8, "[Falkon Linux Console TTY1] - Type 'startx' to launch Desktop GUI Window Manager");
            
            terminal_instance_t* tty = (terminal_instance_t*)terminal_get_state();
            terminal_instance_render(tty, 20, 40);
            
            swap_buffers();
        }

        // Frame pacing delay
        timer_wait(1);
    }
}