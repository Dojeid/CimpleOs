#include "desktop.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "kernel/timer.h"
#include "mm/pmm.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/terminal.h"
#include "gui/apps/installer.h"
#include "gui/apps/settings.h"
#include "gui/apps/file_explorer.h"
#include "gui/apps/notepad.h"
#include "gui/apps/sysmon.h"
#include "gui/apps/calc.h"

static desktop_t desktop;

void desktop_init() {
    desktop.bg_color = 0x0B0F19;  // Deep Obsidian
    desktop.topbar_color = 0x0F172A;  // Dark Slate
    desktop.show_wallpaper = 1;
    desktop.active_theme_id = 1;
}

void desktop_render_background() {
    extern int screen_w, screen_h;
    int desktop_h = screen_h - DESKTOP_TOPBAR_HEIGHT - DESKTOP_TASKBAR_HEIGHT;
    
    // Main background fill
    draw_rect(0, DESKTOP_TOPBAR_HEIGHT, screen_w, desktop_h, desktop.bg_color);
    
    // Geometric Wallpaper Pattern & Gradients based on Active Theme
    uint32_t grid_col = 0x1E293B;
    uint32_t accent_badge = 0x38BDF8;

    switch (desktop.active_theme_id) {
        case 2: // Cyber Blue
            grid_col = 0x1E3A8A;
            accent_badge = 0x60A5FA;
            break;
        case 3: // Emerald Forest
            grid_col = 0x065F46;
            accent_badge = 0x34D399;
            break;
        case 4: // Sunset Purple
            grid_col = 0x581C87;
            accent_badge = 0xC084FC;
            break;
        case 5: // Synthwave Neon
            grid_col = 0x831843;
            accent_badge = 0xF472B6;
            break;
        case 1:
        default: // Midnight Slate
            grid_col = 0x1E293B;
            accent_badge = 0x38BDF8;
            break;
    }

    // Aesthetic grid accents
    for (int y = DESKTOP_TOPBAR_HEIGHT + 50; y < screen_h - DESKTOP_TASKBAR_HEIGHT; y += 60) {
        draw_rect(0, y, screen_w, 1, grid_col);
    }
    for (int x = 110; x < screen_w; x += 100) {
        draw_rect(x, DESKTOP_TOPBAR_HEIGHT, 1, desktop_h, grid_col);
    }

    // Centered Falkon-OS Desktop Watermark Badge
    int center_x = (screen_w / 2) - 150;
    int center_y = (screen_h / 2) - 30;
    draw_rect(center_x, center_y, 300, 56, 0x0F172A);
    draw_rect(center_x + 2, center_y + 2, 296, 52, 0x1E293B);
    draw_rect(center_x + 2, center_y + 2, 296, 2, accent_badge);
    draw_string(center_x + 35, center_y + 16, accent_badge, "FALKON-OS ENTERPRISE 64-BIT");
    draw_string(center_x + 45, center_y + 32, 0x94A3B8, "EXT4 System & Modern Desktop");

    // Desktop Application Shortcuts (Left Sidebar Grid)
    // Shortcut 1: OS Installer
    draw_rect(20, 45, 75, 55, 0x1E293B);
    draw_rect(20, 45, 75, 3, 0x10B981);
    draw_rect(38, 55, 38, 22, 0x059669);
    draw_string(45, 60, 0xFFFFFF, "HDD");
    draw_string(14, 104, 0xFFFFFF, "Install OS");

    // Shortcut 2: Settings
    draw_rect(20, 125, 75, 55, 0x1E293B);
    draw_rect(20, 125, 75, 3, 0x0284C7);
    draw_rect(38, 135, 38, 22, 0x0369A1);
    draw_string(44, 140, 0xFFFFFF, "SET");
    draw_string(18, 184, 0xFFFFFF, "Settings");

    // Shortcut 3: Terminal
    draw_rect(20, 205, 75, 55, 0x1E293B);
    draw_rect(20, 205, 75, 3, 0xF59E0B);
    draw_rect(38, 215, 38, 22, 0xD97706);
    draw_string(45, 220, 0xFFFFFF, ">_");
    draw_string(18, 264, 0xFFFFFF, "Terminal");

    // Shortcut 4: File Explorer
    draw_rect(20, 285, 75, 55, 0x1E293B);
    draw_rect(20, 285, 75, 3, 0x38BDF8);
    draw_rect(38, 295, 38, 22, 0x0284C7);
    draw_string(44, 300, 0xFFFFFF, "VFS");
    draw_string(14, 344, 0xFFFFFF, "Explorer");

    // Shortcut 5: Notepad
    draw_rect(20, 365, 75, 55, 0x1E293B);
    draw_rect(20, 365, 75, 3, 0xA855F7);
    draw_rect(38, 375, 38, 22, 0x9333EA);
    draw_string(45, 380, 0xFFFFFF, "TXT");
    draw_string(20, 424, 0xFFFFFF, "Notepad");

    // Shortcut 6: Sysmon
    draw_rect(20, 445, 75, 55, 0x1E293B);
    draw_rect(20, 445, 75, 3, 0xEC4899);
    draw_rect(38, 455, 38, 22, 0xDB2777);
    draw_string(45, 460, 0xFFFFFF, "CPU");
    draw_string(22, 504, 0xFFFFFF, "Sysmon");
}

void desktop_render_topbar() {
    extern int screen_w;
    
    // Top bar background
    draw_rect(0, 0, screen_w, DESKTOP_TOPBAR_HEIGHT, desktop.topbar_color);
    draw_rect(0, DESKTOP_TOPBAR_HEIGHT - 1, screen_w, 1, 0x334155);
    
    // Falkon-OS logo badge
    draw_rect(6, 4, 115, 17, 0x0F172A);
    draw_string(12, 7, 0x38BDF8, "Falkon-OS v1.0");
    
    // Clock calculation (HH:MM:SS format)
    extern volatile uint32_t timer_ticks;
    uint32_t total_seconds = timer_ticks / 100;
    uint32_t hours = (total_seconds / 3600) % 24;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    char timestr[32];
    sprintf(timestr, "%02u:%02u:%02u", hours, minutes, seconds);
    
    // RAM MB calculation
    uint64_t free_mb = pmm_get_free_memory() / (1024 * 1024);
    uint64_t total_mb = pmm_get_total_memory() / (1024 * 1024);
    char ramstr[32];
    sprintf(ramstr, "RAM: %uMB / %uMB", (uint32_t)(total_mb - free_mb), (uint32_t)total_mb);
    
    // Render RAM pill & Clock
    draw_rect(screen_w - 230, 4, 135, 17, 0x0F172A);
    draw_string(screen_w - 224, 7, 0x4ADE80, ramstr);
    
    draw_rect(screen_w - 85, 4, 78, 17, 0x0F172A);
    draw_string(screen_w - 79, 7, 0xF1F5F9, timestr);
}

void desktop_handle_click(int x, int y) {
    if (x >= 15 && x <= 100) {
        // Installer Icon
        if (y >= 40 && y <= 115) {
            installer_open();
        }
        // Settings Icon
        else if (y >= 120 && y <= 195) {
            settings_open();
        }
        // Terminal Icon
        else if (y >= 200 && y <= 275) {
            window_t* term_win = wm_create_window(60, 80, 700, 480, "Terminal");
            if (term_win) {
                term_win->user_data = terminal_get_state();
                taskbar_add_button(term_win->id, "Terminal");
            }
        }
        // File Explorer Icon
        else if (y >= 280 && y <= 355) {
            file_explorer_open();
        }
        // Notepad Icon
        else if (y >= 360 && y <= 435) {
            notepad_open("/docs/welcome.txt");
        }
        // Sysmon Icon
        else if (y >= 440 && y <= 515) {
            sysmon_open();
        }
    }
}

desktop_t* desktop_get_state() {
    return &desktop;
}

void desktop_set_bg_color(uint32_t color) {
    desktop.bg_color = color;
}

void desktop_set_theme(int theme_id) {
    desktop.active_theme_id = theme_id;
    switch (theme_id) {
        case 2: // Cyber Blue
            desktop.bg_color = 0x0F172A;
            desktop.topbar_color = 0x1E3A8A;
            break;
        case 3: // Emerald Forest
            desktop.bg_color = 0x064E3B;
            desktop.topbar_color = 0x047857;
            break;
        case 4: // Sunset Purple
            desktop.bg_color = 0x3B0764;
            desktop.topbar_color = 0x581C87;
            break;
        case 5: // Synthwave Neon
            desktop.bg_color = 0x500724;
            desktop.topbar_color = 0x831843;
            break;
        case 1: // Cyberpunk Midnight Obsidian (Default)
        default:
            desktop.bg_color = 0x0B0F19;
            desktop.topbar_color = 0x0F172A;
            break;
    }
}
