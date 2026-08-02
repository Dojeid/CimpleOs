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
    
    // Deep Obsidian Fill
    draw_rect(0, DESKTOP_TOPBAR_HEIGHT, screen_w, desktop_h, desktop.bg_color);
    
    // Restore Cyber Obsidian Grid Background Pattern
    uint32_t grid_col = 0x1E293B;
    uint32_t accent_badge = 0x38BDF8;

    switch (desktop.active_theme_id) {
        case 2: grid_col = 0x1E3A8A; accent_badge = 0x60A5FA; break;
        case 3: grid_col = 0x065F46; accent_badge = 0x34D399; break;
        case 4: grid_col = 0x581C87; accent_badge = 0xC084FC; break;
        case 5: grid_col = 0x831843; accent_badge = 0xF472B6; break;
        case 1: default: grid_col = 0x1E293B; accent_badge = 0x38BDF8; break;
    }

    // High-tech Cyber Grid Lines
    for (int y = DESKTOP_TOPBAR_HEIGHT + 50; y < screen_h - DESKTOP_TASKBAR_HEIGHT; y += 60) {
        draw_rect(0, y, screen_w, 1, grid_col);
    }
    for (int x = 110; x < screen_w; x += 100) {
        draw_rect(x, DESKTOP_TOPBAR_HEIGHT, 1, desktop_h, grid_col);
    }

    // Centered Falkon-OS Enterprise Watermark Badge
    int center_x = (screen_w / 2) - 160;
    int center_y = (screen_h / 2) - 30;
    draw_rect(center_x, center_y, 320, 56, 0x0F172A);
    draw_rect(center_x + 2, center_y + 2, 316, 52, 0x1E293B);
    draw_rect(center_x + 2, center_y + 2, 316, 2, accent_badge);
    draw_string(center_x + 35, center_y + 16, accent_badge, "FALKON-OS ENTERPRISE 64-BIT");
    draw_string(center_x + 42, center_y + 32, 0x94A3B8, "POSIX & EXT4 Native Subsystem");

    // Desktop Application Shortcuts (Left Grid)
    // 1. Install OS
    draw_rect(20, 45, 80, 58, 0x1E293B);
    draw_rect(20, 45, 80, 3, 0x10B981);
    draw_string(42, 60, 0x10B981, "[HDD]");
    draw_string(14, 107, 0x4ADE80, "Install OS");

    // 2. Settings
    draw_rect(20, 125, 80, 58, 0x1E293B);
    draw_rect(20, 125, 80, 3, 0x0284C7);
    draw_string(42, 140, 0x0284C7, "[SET]");
    draw_string(15, 187, 0xFFFFFF, "Settings");

    // 3. Terminal
    draw_rect(20, 205, 80, 58, 0x1E293B);
    draw_rect(20, 205, 80, 3, 0xF59E0B);
    draw_string(44, 220, 0xF59E0B, ">_");
    draw_string(18, 267, 0xFFFFFF, "Terminal");

    // 4. Explorer
    draw_rect(20, 285, 80, 58, 0x1E293B);
    draw_rect(20, 285, 80, 3, 0x38BDF8);
    draw_string(42, 300, 0x38BDF8, "[VFS]");
    draw_string(18, 347, 0xFFFFFF, "Explorer");

    // 5. Notepad
    draw_rect(20, 365, 80, 58, 0x1E293B);
    draw_rect(20, 365, 80, 3, 0xA855F7);
    draw_string(42, 380, 0xA855F7, "[TXT]");
    draw_string(20, 427, 0xFFFFFF, "Notepad");

    // 6. Sysmon
    draw_rect(20, 445, 80, 58, 0x1E293B);
    draw_rect(20, 445, 80, 3, 0xEC4899);
    draw_string(42, 460, 0xEC4899, "[CPU]");
    draw_string(20, 507, 0xFFFFFF, "Sysmon");
}

void desktop_render_topbar() {
    extern int screen_w;
    extern int installer_is_system_installed(void);
    
    // Topbar background
    draw_rect(0, 0, screen_w, DESKTOP_TOPBAR_HEIGHT, desktop.topbar_color);
    draw_rect(0, DESKTOP_TOPBAR_HEIGHT - 1, screen_w, 1, 0x334155);
    
    // Falkon-OS Enterprise Badge
    draw_rect(6, 4, 135, 17, 0x0F172A);
    draw_string(12, 7, 0x38BDF8, "Falkon-OS v1.0");

    // Mode Status Pill (Live ISO Preview vs Native Installed Disk)
    int is_installed = installer_is_system_installed();
    uint32_t pill_bg = is_installed ? 0x059669 : 0xF59E0B;
    draw_rect(150, 4, is_installed ? 250 : 275, 17, pill_bg);
    draw_string(156, 7, is_installed ? 0xFFFFFF : 0x000000, 
                is_installed ? "[ INSTALLED NATIVE DISK: /dev/sda1 ]" : "[ LIVE USB PREVIEW - Double Click 'Install OS' ]");
    
    // Live Timer Telemetry
    extern volatile uint32_t timer_ticks;
    uint32_t total_seconds = timer_ticks / 100;
    uint32_t hours = (total_seconds / 3600) % 24;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    char timestr[32];
    sprintf(timestr, "%02u:%02u:%02u", hours, minutes, seconds);
    
    // Physical RAM Telemetry
    uint64_t free_mb = pmm_get_free_memory() / (1024 * 1024);
    uint64_t total_mb = pmm_get_total_memory() / (1024 * 1024);
    char ramstr[32];
    sprintf(ramstr, "RAM: %uMB / %uMB", (uint32_t)(total_mb - free_mb), (uint32_t)total_mb);
    
    // Status Badges
    draw_rect(screen_w - 230, 4, 135, 17, 0x0F172A);
    draw_string(screen_w - 224, 7, 0x4ADE80, ramstr);
    
    draw_rect(screen_w - 85, 4, 78, 17, 0x0F172A);
    draw_string(screen_w - 79, 7, 0xF1F5F9, timestr);
}

void desktop_handle_click(int x, int y) {
    if (x >= 15 && x <= 105) {
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
        // Explorer Icon
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

static gui_theme_t active_gui_theme = {
    .bg_color = 0x0B0F19,
    .topbar_color = 0x0F172A,
    .taskbar_color = 0x0F172A,
    .titlebar_active = 0x1E293B,
    .titlebar_inactive = 0x0F172A,
    .accent_color = 0x0284C7,
    .text_primary = 0xF1F5F9
};

gui_theme_t* theme_get_current(void) {
    return &active_gui_theme;
}

void desktop_set_bg_color(uint32_t color) {
    desktop.bg_color = color;
    active_gui_theme.bg_color = color;
}

void desktop_set_theme(int theme_id) {
    desktop.active_theme_id = theme_id;
    switch (theme_id) {
        case 2: // Cyber Blue
            active_gui_theme.bg_color = 0x0A192F;
            active_gui_theme.topbar_color = 0x112240;
            active_gui_theme.taskbar_color = 0x112240;
            active_gui_theme.titlebar_active = 0x1D3557;
            active_gui_theme.titlebar_inactive = 0x112240;
            active_gui_theme.accent_color = 0x60A5FA;
            break;
        case 3: // Emerald Forest
            active_gui_theme.bg_color = 0x064E3B;
            active_gui_theme.topbar_color = 0x065F46;
            active_gui_theme.taskbar_color = 0x065F46;
            active_gui_theme.titlebar_active = 0x047857;
            active_gui_theme.titlebar_inactive = 0x065F46;
            active_gui_theme.accent_color = 0x34D399;
            break;
        case 4: // Sunset Purple
            active_gui_theme.bg_color = 0x3B0764;
            active_gui_theme.topbar_color = 0x581C87;
            active_gui_theme.taskbar_color = 0x581C87;
            active_gui_theme.titlebar_active = 0x6B21A8;
            active_gui_theme.titlebar_inactive = 0x581C87;
            active_gui_theme.accent_color = 0xC084FC;
            break;
        case 5: // Synthwave Neon
            active_gui_theme.bg_color = 0x500724;
            active_gui_theme.topbar_color = 0x831843;
            active_gui_theme.taskbar_color = 0x831843;
            active_gui_theme.titlebar_active = 0x9D174D;
            active_gui_theme.titlebar_inactive = 0x831843;
            active_gui_theme.accent_color = 0xF472B6;
            break;
        case 1: // Cyberpunk Midnight Obsidian (Default)
        default:
            active_gui_theme.bg_color = 0x0B0F19;
            active_gui_theme.topbar_color = 0x0F172A;
            active_gui_theme.taskbar_color = 0x0F172A;
            active_gui_theme.titlebar_active = 0x1E293B;
            active_gui_theme.titlebar_inactive = 0x0F172A;
            active_gui_theme.accent_color = 0x0284C7;
            break;
    }
    desktop.bg_color = active_gui_theme.bg_color;
    desktop.topbar_color = active_gui_theme.topbar_color;
}
