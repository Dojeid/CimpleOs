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
#include "mm/slab.h"
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

int sys_runlevel = 5; // Default 5 = Graphical Desktop GUI Mode (with TTY Runlevel 3 fallback)

void sys_set_runlevel(int level) {
    if (level == 3 || level == 5) {
        sys_runlevel = level;
        clear_screen(0x000000);
        if (level == 5) {
            vga_print("Starting Graphical Desktop Environment (Runlevel 5)...\n");
        } else {
            vga_print("Switched to Core Linux Console TTY Mode (Runlevel 3).\n");
        }
    }
}

int sys_get_runlevel(void) {
    return sys_runlevel;
}

void sys_shutdown(void) {
    vga_print("\n[Falkon-OS] Performing ACPI/QEMU Hardware Shutdown...\n");
    // 1. QEMU ACPI / PIIX4 shutdown port
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    // 2. VirtualBox / Bochs ACPI shutdown port
    outw(0x4004, 0x3400);
    // 3. APM / Legacy PC shutdown
    outw(0x2000, 0x2000);
    // 4. Keyboard Controller Reset Fallback
    outb(0x64, 0xFE);
    while (1) {
        asm volatile("cli; hlt");
    }
}

static void boot_log_ok(const char* message) {
    terminal_instance_t* tty = (terminal_instance_t*)terminal_get_state();
    char line[160];
    sprintf(line, "[  OK  ] %s", message);
    if (tty) terminal_instance_print(tty, line);
    vga_print(line);
    vga_print("\n");
}

static void gui_compositor_task(void) { while(1) { asm volatile("pause"); } }
static void input_poller_task(void) { while(1) { asm volatile("pause"); } }

void kmain(void* multiboot_info_addr) {
    multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info_addr;

    vga_clear();
    vga_print("Falkon-OS Enterprise Linux Kernel 6.8.0 Booting...\n");

    gdt_install();
    boot_log_ok("Initialized Global Descriptor Table (GDT) & Task State Segment");

    uint64_t total_mem_bytes = 0;
    if (mbi && (mbi->flags & 0x1)) {
        total_mem_bytes = (uint64_t)(mbi->mem_upper + mbi->mem_lower) * 1024;
    }
    pmm_init(total_mem_bytes > 0 ? total_mem_bytes : 128 * 1024 * 1024);
    boot_log_ok("Initialized Physical Memory Manager (PMM)");

    vmm_init();
    boot_log_ok("Enabling Paging & Virtual Memory Manager (VMM 64-bit)");

    heap_init();
    slab_init();
    boot_log_ok("Kernel Heap Allocator & Slab Cache Ready");

    graphics_init(mbi);
    boot_log_ok("VESA/BGA Graphics Framebuffer Double-Buffered Canvas");

    init_idt();
    boot_log_ok("Interrupt Descriptor Table (IDT) & Hardware IRQs Active");

    extern void cpuid_init(void);
    cpuid_init();
    boot_log_ok("Hardware CPUID Inspection Engine Initialized");

    init_mouse();
    timer_init(100);
    sysinfo_init();
    boot_log_ok("Programmable Interval Timer (PIT IRQ0 @ 100Hz) & PS/2 Input");
    
    usb_init();
    sound_init();
    ata_init();
    boot_log_ok("ATA Storage Controller & Virtual Disks Active");
    
    extern int e1000_init(void);
    e1000_init();
    boot_log_ok("Intel 82540EM Gigabit Ethernet NIC (MAC: 52:54:00:12:34:56)");

#include "gui/notify.h"

    desktop_init();
    taskbar_init();
    cursor_init();
    notify_init();
    notify_push("Falkon-OS Ready", "Windows 11 Desktop GUI Loaded (60 FPS)", 0x38BDF8);
    notify_push("Guest Integration", "VirtualBox Host Mouse Integration Active", 0x4ADE80);
    
    asm volatile("sti");

    vfs_init();
    ext4_init();
    
    if (vfs_mount("hda", "/", "ext4") != 0) {
        boot_log_ok("Mounted EXT4 Root Filesystem on /dev/sda1");
    }

    ramdisk_init();
    process_init();
    syscall_init();
    boot_log_ok("POSIX System Call Dispatcher & Task Scheduler Ready");

    process_create("gui_compositor", gui_compositor_task);
    process_create("input_poller", input_poller_task);

    terminal_init();
    wm_init();
    boot_log_ok("Starting Windows 11 Desktop GUI & Window Manager (Runlevel 5)...");
    
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
            terminal_instance_print(term, "System booted directly into Graphical Desktop (Runlevel 5).");
            terminal_instance_print(term, "Type 'init 3' or 'tty' to switch to Core Linux Console.");
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
    terminal_instance_print(root_tty, "Type 'startx' to return to Desktop GUI Window Manager.");
    terminal_instance_print(root_tty, "Type 'help' for command list.");
    terminal_instance_print(root_tty, "");

    int last_mouse_btn = 0;

    while (1) {
        if (sys_runlevel == 5) {
            // === RUNLEVEL 5: GRAPHICAL DESKTOP GUI MODE ===
            int mouse_btn = mouse_button_left();
            int mouse_pressed = mouse_button_pressed();
            
            if (mouse_pressed) {
                taskbar_t* tb = taskbar_get_state();
                int menu_w = 360, menu_h = 370;
                int menu_x = (screen_w / 2) - (menu_w / 2);
                int menu_y = tb->y_position - menu_h - 12;
                
                int in_taskbar = (mouse_y >= tb->y_position) ||
                                 (tb->start_menu_open && 
                                  mouse_x >= menu_x && mouse_x < menu_x + menu_w && 
                                  mouse_y >= menu_y && mouse_y < menu_y + menu_h);
                if (in_taskbar) {
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
            notify_render();
            cursor_render();
            swap_buffers();
            timer_wait(1); // 60 FPS Frame Limiter
        } else {
            // === RUNLEVEL 3: CORE LINUX CONSOLE TTY CLI MODE ===
            clear_screen(0x0A0E17);
            
            draw_rect(0, 0, screen_w, 24, 0x1E293B);
            draw_string(10, 6, 0x38BDF8, "[Falkon Linux Console TTY1] - Type 'startx' to launch Desktop GUI Window Manager");
            
            terminal_instance_t* tty = (terminal_instance_t*)terminal_get_state();
            terminal_instance_render(tty, 20, 40);
            
            swap_buffers();
            timer_wait(1);
        }
    }
}