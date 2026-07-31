#include "desktop.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "kernel/timer.h"
#include "mm/pmm.h"

static desktop_t desktop;

void desktop_init() {
    desktop.bg_color = 0x1E1E1E;  // Dark gray
    desktop.topbar_color = 0x2C3E50;  // Blue-gray
    desktop.show_wallpaper = 0;
}

void desktop_render_background() {
    extern int screen_w, screen_h;
    
    int desktop_h = screen_h - DESKTOP_TOPBAR_HEIGHT - DESKTOP_TASKBAR_HEIGHT;
    
    // Main solid background
    draw_rect(0, DESKTOP_TOPBAR_HEIGHT, screen_w, desktop_h, desktop.bg_color);
    
    // Aesthetic geometric grid accents
    uint32_t line_color = (desktop.bg_color == 0x1E1E1E) ? 0x2A2A2A : (desktop.bg_color + 0x0A0A0A);
    for (int y = DESKTOP_TOPBAR_HEIGHT + 40; y < screen_h - DESKTOP_TASKBAR_HEIGHT; y += 60) {
        draw_rect(0, y, screen_w, 1, line_color);
    }
    for (int x = 40; x < screen_w; x += 80) {
        draw_rect(x, DESKTOP_TOPBAR_HEIGHT, 1, desktop_h, line_color);
    }

    // Centered Falkon OS Watermark Accent Box
    int center_x = (screen_w / 2) - 140;
    int center_y = (screen_h / 2) - 40;
    draw_rect(center_x, center_y, 280, 50, 0x111827);
    draw_rect(center_x + 2, center_y + 2, 276, 46, 0x1F2937);
    draw_string(center_x + 25, center_y + 16, 0x38BDF8, "FALKON-OS 64-BIT GUI");
}

void desktop_render_topbar() {
    extern int screen_w;
    
    // Top bar background
    draw_rect(0, 0, screen_w, DESKTOP_TOPBAR_HEIGHT, desktop.topbar_color);
    
    // Falkon-OS logo text
    draw_string(8, 7, 0xECF0F1, "Falkon-OS v0.4 GUI");
    
    // System info on right side
    char timestr[32];
    
    // BUG FIX: Clock display with HH:MM:SS format (supports up to 99:59:59)
    extern volatile uint32_t timer_ticks;
    uint32_t total_seconds = timer_ticks / 100;
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    // Cap hours at 99
    if (hours > 99) hours = 99;
    
    // Format: HH:MM:SS
    timestr[0] = '0' + (hours / 10);
    timestr[1] = '0' + (hours % 10);
    timestr[2] = ':';
    timestr[3] = '0' + (minutes / 10);
    timestr[4] = '0' + (minutes % 10);
    timestr[5] = ':';
    timestr[6] = '0' + (seconds / 10);
    timestr[7] = '0' + (seconds % 10);
    timestr[8] = '\0';
    
    // UX FIX: Show RAM next to uptime for better layout
    uint64_t free_mb = pmm_get_free_memory() / (1024 * 1024);
    uint64_t total_mb = pmm_get_total_memory() / (1024 * 1024);
    
    char ramstr[32];
    int idx = 0;
    ramstr[idx++] = 'R';
    ramstr[idx++] = 'A';
    ramstr[idx++] = 'M';
    ramstr[idx++] = ':';
    ramstr[idx++] = ' ';
    if (free_mb >= 100) ramstr[idx++] = '0' + (free_mb / 100);
    if (free_mb >= 10) ramstr[idx++] = '0' + ((free_mb / 10) % 10);
    ramstr[idx++] = '0' + (free_mb % 10);
    ramstr[idx++] = '/';
    if (total_mb >= 100) ramstr[idx++] = '0' + (total_mb / 100);
    if (total_mb >= 10) ramstr[idx++] = '0' + ((total_mb / 10) % 10);
    ramstr[idx++] = '0' + (total_mb % 10);
    ramstr[idx++] = 'M';
    ramstr[idx++] = '\0';
    
    // Draw both: RAM then uptime
    draw_string(screen_w - 180, 7, 0xECF0F1, ramstr);
    draw_string(screen_w - 80, 7, 0xECF0F1, timestr);
}

desktop_t* desktop_get_state() {
    return &desktop;
}

void desktop_set_bg_color(uint32_t color) {
    desktop.bg_color = color;
}

void desktop_set_theme(int theme_id) {
    switch (theme_id) {
        case 2: // Cyber Blue
            desktop.bg_color = 0x0F172A;
            desktop.topbar_color = 0x1E293B;
            break;
        case 3: // Emerald Forest
            desktop.bg_color = 0x064E3B;
            desktop.topbar_color = 0x047857;
            break;
        case 4: // Sunset Purple
            desktop.bg_color = 0x3B0764;
            desktop.topbar_color = 0x581C87;
            break;
        case 1: // Midnight Dark (Default)
        default:
            desktop.bg_color = 0x1E1E1E;
            desktop.topbar_color = 0x2C3E50;
            break;
    }
}
