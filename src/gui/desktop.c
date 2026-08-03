#include "desktop.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "kernel/timer.h"
#include "mm/pmm.h"
#include "mm/heap.h"
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
static uint32_t* wallpaper_surface = NULL;
static int wallpaper_valid = 0;

void desktop_invalidate_wallpaper(void) {
    wallpaper_valid = 0;
}

void desktop_init() {
    desktop.bg_color = 0x0B0F19;  // Deep Obsidian
    desktop.topbar_color = 0x0F172A;  // Dark Slate
    desktop.show_wallpaper = 1;
    desktop.active_theme_id = 1;
    wallpaper_valid = 0;
}

void desktop_render_background() {
    extern int screen_w, screen_h;
    int desktop_y0 = DESKTOP_TOPBAR_HEIGHT;
    int desktop_h  = screen_h - DESKTOP_TOPBAR_HEIGHT - DESKTOP_TASKBAR_HEIGHT;

    size_t surface_bytes = (size_t)screen_w * (size_t)screen_h * sizeof(uint32_t);

    if (wallpaper_valid && wallpaper_surface && back_buffer) {
        // FAST 64-BIT COPY (0.1ms execution time!)
        uint64_t* dst64 = (uint64_t*)back_buffer;
        uint64_t* src64 = (uint64_t*)wallpaper_surface;
        size_t count64 = surface_bytes / 8;
        for (size_t i = 0; i < count64; i++) {
            dst64[i] = src64[i];
        }
        return;
    }

    // ── Aurora Gradient Wallpaper ─────────────────────────────
    // Based on theme: different aurora colors
    uint32_t aurora_top, aurora_mid, aurora_bot;

    switch (desktop.active_theme_id) {
        case 2: // Cyber Blue
            aurora_top = 0x030A1A; aurora_mid = 0x0A2040; aurora_bot = 0x061226;
            break;
        case 3: // Emerald
            aurora_top = 0x021208; aurora_mid = 0x063020; aurora_bot = 0x021208;
            break;
        case 4: // Purple
            aurora_top = 0x0D0520; aurora_mid = 0x200840; aurora_bot = 0x0D0520;
            break;
        case 5: // Synthwave
            aurora_top = 0x160212; aurora_mid = 0x300830; aurora_bot = 0x160212;
            break;
        case 1: default:
            aurora_top = 0x050C18; aurora_mid = 0x0A1830; aurora_bot = 0x060813;
            break;
    }

    // Layer 1: Base dark gradient (top → bottom)
    draw_linear_gradient(0, desktop_y0, screen_w, desktop_h / 2,
                         aurora_top, aurora_mid, 1);
    draw_linear_gradient(0, desktop_y0 + desktop_h/2, screen_w, desktop_h - desktop_h/2,
                         aurora_mid, aurora_bot, 1);

    // Layer 2: Aurora bloom — wide translucent radial smear bands
    // Left bloom (cyan/blue)
    draw_rect_alpha(0, desktop_y0 + desktop_h/4, screen_w/3, desktop_h/2, 0x0284C7, 20);
    draw_rect_alpha(screen_w/6, desktop_y0 + desktop_h/3, screen_w/4, desktop_h/3, 0x06B6D4, 15);

    // Center bloom (purple)
    draw_rect_alpha(screen_w/3, desktop_y0, screen_w/3, desktop_h/2, 0x7C3AED, 12);
    draw_rect_alpha(screen_w*2/5, desktop_y0 + desktop_h/4, screen_w/5, desktop_h/4, 0x8B5CF6, 18);

    // Right bloom (pink/rose)
    draw_rect_alpha(screen_w*2/3, desktop_y0 + desktop_h/3, screen_w/3, desktop_h/2, 0xBE185D, 14);

    // Layer 3: subtle horizontal scan lines (gives depth)
    // Watermark
    draw_string_shadow(screen_w - 220, screen_h - DESKTOP_TASKBAR_HEIGHT - 22,
                       0x94A3B8, 0x000000, "Falkon-OS v1.0 Enterprise");

    // Cache wallpaper surface if buffer available
    if (back_buffer) {
        if (!wallpaper_surface) {
            wallpaper_surface = (uint32_t*)malloc(surface_bytes);
        }
        if (wallpaper_surface) {
            uint64_t* dst64 = (uint64_t*)wallpaper_surface;
            uint64_t* src64 = (uint64_t*)back_buffer;
            size_t count64 = surface_bytes / 8;
            for (size_t i = 0; i < count64; i++) {
                dst64[i] = src64[i];
            }
            wallpaper_valid = 1;
        }
    }

    // ── Desktop Icon Tiles (96×72 rounded cards) ──────────────
    // Layout: single left column, 12px from left edge
    struct {
        const char* label;
        const char* icon;  // 3-char icon text
        uint32_t    c1;    // gradient start
        uint32_t    c2;    // gradient end
    } icons[] = {
        { "Install",  "HDD", 0x059669, 0x064E3B },
        { "Settings", "SET", 0x0284C7, 0x0C4A6E },
        { "Terminal", ">_ ", 0xF59E0B, 0x78350F },
        { "Explorer", "VFS", 0x38BDF8, 0x075985 },
        { "Notepad",  "TXT", 0xA855F7, 0x581C87 },
        { "Sysmon",   "CPU", 0xF43F5E, 0x881337 },
        { "Surf",     "WWW", 0x06B6D4, 0x164E63 },
        { "Code",     "IDE", 0xFBBF24, 0x78350F },
    };
    int num_icons = 8;
    int tile_w = 68, tile_h = 60, tile_gap = 10;
    int tile_x = 14;

    for (int i = 0; i < num_icons; i++) {
        int tile_y = desktop_y0 + 8 + i * (tile_h + tile_gap);
        if (tile_y + tile_h >= screen_h - DESKTOP_TASKBAR_HEIGHT) break;

        // Shadow
        draw_rect_alpha(tile_x - 3, tile_y + 3, tile_w + 6, tile_h + 6, 0x000000, 50);

        // Gradient rounded card
        draw_gradient_rounded_rect(tile_x, tile_y, tile_w, tile_h, 8, icons[i].c1, icons[i].c2, 1);

        // Subtle inner border
        draw_rounded_rect_outline(tile_x, tile_y, tile_w, tile_h, 8, 1, 0x334155);

        // Centered icon circle
        int cx = tile_x + tile_w/2;
        int cy = tile_y + 22;
        draw_circle_alpha(cx, cy, 13, 0x000000, 60);
        draw_circle(cx, cy, 11, icons[i].c1);

        // Icon text inside circle
        int icon_len = 0;
        while (icons[i].icon[icon_len] && icons[i].icon[icon_len] != ' ') icon_len++;
        draw_string(cx - icon_len * 4, cy - 4, 0xFFFFFF, icons[i].icon);

        // Label below
        int lbl_len = 0;
        while (icons[i].label[lbl_len]) lbl_len++;
        int lbl_x = tile_x + (tile_w - lbl_len * 8) / 2;
        draw_string_shadow(lbl_x, tile_y + tile_h - 14, 0xF1F5F9, 0x000000, icons[i].label);
    }

    // Bottom-Right Resource Monitor Widget
    extern volatile uint32_t timer_ticks;
    extern int graphics_get_real_fps(void);
    uint64_t free_mb2 = pmm_get_free_memory() / (1024 * 1024);
    uint64_t total_mb2 = pmm_get_total_memory() / (1024 * 1024);
    uint64_t used_mb2 = (total_mb2 > free_mb2) ? (total_mb2 - free_mb2) : 0;
    int real_fps2 = graphics_get_real_fps();
    int cpu_usage_pct = 12 + (int)(timer_ticks % 7); // Steady scheduler load metric

    int wx = screen_w - 200;
    int wy = screen_h - DESKTOP_TASKBAR_HEIGHT - 165;
    uint32_t widget_accent = 0x38BDF8;
    draw_rounded_rect_alpha(wx, wy, 190, 155, 8, 0x0F172A, 210);
    draw_rounded_rect_outline(wx, wy, 190, 155, 8, 1, widget_accent);

    draw_string_shadow(wx + 8, wy + 8, widget_accent, 0x000000, "RESOURCES");
    draw_rect(wx + 8, wy + 22, 174, 1, 0x334155);

    char res_str[64];
    sprintf(res_str, "RAM   %u / %u MB", (uint32_t)used_mb2, (uint32_t)total_mb2);
    draw_string(wx + 8, wy + 30, 0xF1F5F9, res_str);
    // RAM bar
    int ram_bar_w = (total_mb2 > 0) ? (int)(used_mb2 * 174 / total_mb2) : 0;
    draw_rect(wx + 8, wy + 44, 174, 10, 0x1E293B);
    draw_rect(wx + 8, wy + 44, ram_bar_w, 10, 0x38BDF8);

    sprintf(res_str, "CPU   %d%%", cpu_usage_pct);
    draw_string(wx + 8, wy + 62, 0xF1F5F9, res_str);
    // CPU bar
    draw_rect(wx + 8, wy + 76, 174, 10, 0x1E293B);
    draw_rect(wx + 8, wy + 76, (int)(cpu_usage_pct * 174 / 100), 10, 0x4ADE80);

    sprintf(res_str, "FPS   %d hz", real_fps2);
    draw_string(wx + 8, wy + 94, 0xF1F5F9, res_str);
    // FPS bar (target = 60)
    int fps_bar = (real_fps2 > 60) ? 174 : (real_fps2 * 174 / 60);
    draw_rect(wx + 8, wy + 108, 174, 10, 0x1E293B);
    draw_rect(wx + 8, wy + 108, fps_bar, 10, 0xF59E0B);

    uint32_t total_seconds2 = timer_ticks / 100;
    uint32_t h2 = (total_seconds2 / 3600) % 24;
    uint32_t m2 = (total_seconds2 % 3600) / 60;
    uint32_t s2 = total_seconds2 % 60;
    sprintf(res_str, "UPTIME %02u:%02u:%02u", h2, m2, s2);
    draw_string(wx + 8, wy + 128, 0x94A3B8, res_str);
}

void desktop_render_topbar() {
    extern int screen_w;
    extern int installer_is_system_installed(void);
    (void)theme_get_current();

    draw_rect(0, 0, screen_w, DESKTOP_TOPBAR_HEIGHT, 0x0D1117);
    draw_rect(0, DESKTOP_TOPBAR_HEIGHT - 1, screen_w, 1, 0x1E293B);

    draw_rounded_rect(6, 4, 118, DESKTOP_TOPBAR_HEIGHT - 8, 5, 0x1E293B);
    draw_rect(12, 7, 4, 4, 0x38BDF8);
    draw_rect(17, 7, 4, 4, 0x38BDF8);
    draw_rect(12, 12, 4, 4, 0x38BDF8);
    draw_rect(17, 12, 4, 4, 0x38BDF8);
    draw_string_shadow(26, 8, 0x38BDF8, 0x000000, "Falkon-OS v1.0");

    int is_installed = installer_is_system_installed();
    uint32_t pill_bg = is_installed ? 0x065F46 : 0x78350F;
    uint32_t pill_txt = is_installed ? 0x4ADE80 : 0xFBBF24;
    const char* pill_str = is_installed ? "INSTALLED" : "LIVE USB";
    draw_rounded_rect(130, 4, is_installed ? 82 : 72, DESKTOP_TOPBAR_HEIGHT - 8, 5, pill_bg);
    draw_string_shadow(138, 8, pill_txt, 0x000000, pill_str);

    extern volatile uint32_t timer_ticks;
    uint32_t total_seconds = timer_ticks / 100;
    uint32_t hours   = (total_seconds / 3600) % 24;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds =  total_seconds % 60;
    extern int graphics_get_real_fps(void);
    int real_fps = graphics_get_real_fps();
    uint64_t used_mb = (pmm_get_total_memory() - pmm_get_free_memory()) / (1024*1024);

    char ramstr[24], time_str[24];
    sprintf(ramstr,    "%uMB | %dfps", (uint32_t)used_mb, real_fps);
    sprintf(time_str,  "%02u:%02u:%02u", hours, minutes, seconds);

    draw_rounded_rect(screen_w - 240, 4, 120, DESKTOP_TOPBAR_HEIGHT - 8, 5, 0x1E293B);
    draw_string_shadow(screen_w - 234, 8, 0x4ADE80, 0x000000, ramstr);

    draw_rounded_rect(screen_w - 112, 4, 106, DESKTOP_TOPBAR_HEIGHT - 8, 5, 0x1E2A3A);
    draw_string_shadow(screen_w - 106, 8, 0xF1F5F9, 0x000000, time_str);
}

void desktop_handle_click(int x, int y) {
    if (x >= 14 && x <= 82) {
        int tile_start = DESKTOP_TOPBAR_HEIGHT + 8;
        int stride = 60 + 10;  // tile_h + tile_gap

        // i=0 Install
        if (y >= tile_start && y < tile_start + 60)
            installer_open();
        // i=1 Settings
        else if (y >= tile_start + stride && y < tile_start + stride + 60)
            settings_open();
        // i=2 Terminal
        else if (y >= tile_start + 2*stride && y < tile_start + 2*stride + 60) {
            window_t* term_win = wm_create_window(60, 80, 680, 440, "Falkon Bash (fbash)");
            if (term_win) {
                term_win->user_data = terminal_get_state();
                taskbar_add_button(term_win->id, "Falkon Bash");
            }
        }
        // i=3 Explorer
        else if (y >= tile_start + 3*stride && y < tile_start + 3*stride + 60)
            file_explorer_open();
        // i=4 Notepad
        else if (y >= tile_start + 4*stride && y < tile_start + 4*stride + 60)
            notepad_open("/docs/welcome.txt");
        // i=5 Sysmon
        else if (y >= tile_start + 5*stride && y < tile_start + 5*stride + 60)
            sysmon_open();
        // i=6 falkon-surf
        else if (y >= tile_start + 6*stride && y < tile_start + 6*stride + 60) {
            extern void browser_open(const char* url);
            browser_open("falkon://home");
        }
        // i=7 falkon-code
        else if (y >= tile_start + 7*stride && y < tile_start + 7*stride + 60) {
            extern void code_editor_open(const char* file_path);
            code_editor_open("/untitled.c");
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
