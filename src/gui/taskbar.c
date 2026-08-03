#include "taskbar.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "lib/io.h"
#include "lib/printf.h"
#include "gui/window_manager.h"
#include "gui/terminal.h"
#include "mm/pmm.h"
#include "kernel/timer.h"
#include "gui/apps/file_explorer.h"
#include "gui/apps/notepad.h"
#include "gui/apps/sysmon.h"
#include "gui/apps/calc.h"
#include "gui/apps/settings.h"
#include "gui/apps/installer.h"
#include "gui/desktop.h"

#define COLOR_TASKBAR_BG      0x0F172A
#define COLOR_BUTTON_BG       0x1E293B
#define COLOR_BUTTON_ACTIVE   0x0284C7

static taskbar_t taskbar;
static launcher_button_t launcher_btn;
static int quick_settings_open = 0;

void taskbar_init() {
    extern int screen_h;
    
    taskbar.button_count = 0;
    taskbar.y_position = screen_h - TASKBAR_HEIGHT;
    taskbar.start_menu_open = 0;
    quick_settings_open = 0;
    
    for (int i = 0; i < MAX_TASKBAR_BUTTONS; i++) {
        taskbar.buttons[i].window_id = -1;
    }
    
    launcher_btn.x = 0;
    launcher_btn.width = 95;
    strcpy(launcher_btn.label, "[#] Start");
    launcher_btn.enabled = 1;
}

void taskbar_add_button(int window_id, const char* label) {
    if (taskbar.button_count >= MAX_TASKBAR_BUTTONS) return;
    
    for (int i = 0; i < taskbar.button_count; i++) {
        if (taskbar.buttons[i].window_id == window_id) {
            return;
        }
    }
    
    taskbar_button_t* btn = &taskbar.buttons[taskbar.button_count];
    btn->window_id = window_id;
    strncpy(btn->label, label, 31);
    btn->label[31] = '\0';
    
    taskbar.button_count++;
}

void taskbar_remove_button(int window_id) {
    for (int i = 0; i < taskbar.button_count; i++) {
        if (taskbar.buttons[i].window_id == window_id) {
            for (int j = i; j < taskbar.button_count - 1; j++) {
                taskbar.buttons[j] = taskbar.buttons[j + 1];
            }
            taskbar.button_count--;
            taskbar.buttons[taskbar.button_count].window_id = -1;
            return;
        }
    }
}

void taskbar_render() {
    extern int screen_w, screen_h;
    gui_theme_t* theme = theme_get_current();

    // ── Taskbar Background ────────────────────────────────────
    // Acrylic dark glass base (slightly lighter than desktop)
    draw_rect(0, taskbar.y_position, screen_w, TASKBAR_HEIGHT, 0x0D1117);
    // Top separator line (accent-colored hairline)
    draw_rect(0, taskbar.y_position, screen_w, 1, 0x1E293B);
    // Subtle inner top highlight
    draw_rect(0, taskbar.y_position + 1, screen_w, 1, 0x161D2C);

    // ── Centered Dock Layout ─────────────────────────────────
    // Each icon tile: 40×40 rounded square, 6px gap
    int tile_w = 44, tile_h = 40, tile_gap = 5;
    int start_tile_w = 90;
    int total_dock_w = start_tile_w + tile_gap + taskbar.button_count * (tile_w + tile_gap);
    int dock_start_x = (screen_w - total_dock_w) / 2;
    if (dock_start_x < 10) dock_start_x = 10;

    int tile_y = taskbar.y_position + (TASKBAR_HEIGHT - tile_h) / 2;

    // ── Start Button ─────────────────────────────────────────
    launcher_btn.x = dock_start_x;
    launcher_btn.width = start_tile_w;
    uint32_t start_bg = taskbar.start_menu_open ? theme->accent_color : 0x1E293B;
    // Rounded pill-shaped Start button
    draw_rounded_rect(dock_start_x, tile_y, start_tile_w, tile_h, 8, start_bg);
    if (taskbar.start_menu_open) {
        // Bottom accent bar when menu open
        draw_rect(dock_start_x + 10, taskbar.y_position + TASKBAR_HEIGHT - 3, start_tile_w - 20, 3, theme->accent_color);
    }
    // Windows logo "⊞" approximation: draw 4 colored squares
    int lx = dock_start_x + 10, ly = tile_y + tile_h/2 - 7;
    draw_rect(lx,   ly,   6, 6, 0x38BDF8);   // top-left
    draw_rect(lx+7, ly,   6, 6, 0x38BDF8);   // top-right
    draw_rect(lx,   ly+7, 6, 6, 0x38BDF8);   // bottom-left
    draw_rect(lx+7, ly+7, 6, 6, 0x38BDF8);   // bottom-right
    // "Start" label
    draw_string(dock_start_x + 28, tile_y + tile_h/2 - 4, 0xF1F5F9, "Start");

    // ── App Icon Tiles ────────────────────────────────────────
    int cur_x = dock_start_x + start_tile_w + tile_gap;
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        window_t* win = wm_get_window(btn->window_id);
        if (!win) continue;

        btn->x = cur_x;
        btn->width = tile_w;

        int is_active = (win->flags & WIN_FLAG_FOCUSED);
        int is_min    = (win->flags & WIN_FLAG_MINIMIZED);

        // Tile background
        uint32_t tile_bg = is_active ? 0x0F3460 : (is_min ? 0x111827 : 0x1E293B);
        draw_rounded_rect(cur_x, tile_y, tile_w, tile_h, 8, tile_bg);

        // Active window: accent border + bottom accent bar
        if (is_active) {
            draw_rounded_rect_outline(cur_x, tile_y, tile_w, tile_h, 8, 1, theme->accent_color);
            draw_rect(cur_x + tile_w/2 - 8, taskbar.y_position + TASKBAR_HEIGHT - 4, 16, 3, theme->accent_color);
        }

        // Truncated label centered in tile
        char label_short[8];
        int ll = 0;
        while (btn->label[ll] && ll < 6) { label_short[ll] = btn->label[ll]; ll++; }
        label_short[ll] = '\0';

        int lbl_w = ll * 8;
        int lbl_x = cur_x + (tile_w - lbl_w) / 2;
        uint32_t txt_col = is_active ? 0xF1F5F9 : (is_min ? 0x4B5563 : 0xD1D5DB);
        draw_string(lbl_x, tile_y + tile_h/2 - 4, txt_col, label_short);

        cur_x += tile_w + tile_gap;
    }

    // ── System Tray (right side) ──────────────────────────────
    // Time
    extern volatile uint32_t timer_ticks;
    uint32_t sec  = timer_ticks / 100;
    uint32_t hrs  = (sec / 3600) % 24;
    uint32_t mins = (sec % 3600) / 60;
    char clock_str[16];
    sprintf(clock_str, "%02u:%02u", hrs, mins);

    // Pill-shaped tray container
    int tray_w = 210, tray_h = 36;
    int tray_x = screen_w - tray_w - 12;
    int tray_y2 = taskbar.y_position + (TASKBAR_HEIGHT - tray_h) / 2;
    draw_rounded_rect(tray_x, tray_y2, tray_w, tray_h, 10, 0x161D2C);
    draw_rounded_rect_outline(tray_x, tray_y2, tray_w, tray_h, 10, 1, 0x1E293B);

    // Volume indicator dot
    draw_circle(tray_x + 16, tray_y2 + tray_h/2, 5, 0x22C55E);
    draw_string(tray_x + 24, tray_y2 + tray_h/2 - 4, 0x4ADE80, "VOL");

    // Wi-Fi indicator dot
    draw_circle(tray_x + 66, tray_y2 + tray_h/2, 5, 0x38BDF8);
    draw_string(tray_x + 74, tray_y2 + tray_h/2 - 4, 0x38BDF8, "NET");

    // Clock
    draw_string(tray_x + 118, tray_y2 + tray_h/2 - 4, 0xF1F5F9, clock_str);

    // Notification dot
    draw_circle(tray_x + tray_w - 18, tray_y2 + tray_h/2, 6, 0xF59E0B);
    draw_string(tray_x + tray_w - 22, tray_y2 + tray_h/2 - 4, 0x000000, "3");

    // ── Windows 11 Start Menu Popup ───────────────────────────
    if (taskbar.start_menu_open) {
        int menu_w = 360;
        int menu_h = 370;
        int menu_x = (screen_w / 2) - (menu_w / 2);
        int menu_y = taskbar.y_position - menu_h - 12;

        // Shadow under menu
        draw_rect_alpha(menu_x - 8, menu_y - 4, menu_w + 16, menu_h + 18, 0x000000, 60);

        // Glassmorphic Acrylic Panel (box-blur + overlay)
        draw_box_blur(menu_x, menu_y, menu_w, menu_h, 4);
        draw_rounded_rect(menu_x, menu_y, menu_w, menu_h, 12, 0x0F172A);
        draw_rounded_rect_alpha(menu_x, menu_y, menu_w, menu_h, 12, 0x1E293B, 180);
        // Top accent border
        draw_rounded_rect_outline(menu_x, menu_y, menu_w, menu_h, 12, 1, theme->accent_color);

        // ── Search Bar ─────────────────────────────────────────
        draw_rounded_rect(menu_x + 14, menu_y + 14, menu_w - 28, 28, 8, 0x0B1120);
        draw_rounded_rect_outline(menu_x + 14, menu_y + 14, menu_w - 28, 28, 8, 1, 0x334155);
        // Magnifier icon circle
        draw_circle(menu_x + 28, menu_y + 28, 5, 0x4B5563);
        draw_string(menu_x + 38, menu_y + 21, 0x6B7280, "Search apps, files, settings...");

        // ── Pinned Section ─────────────────────────────────────
        draw_string(menu_x + 16, menu_y + 54, 0xF1F5F9, "Pinned");
        draw_rect(menu_x + 62, menu_y + 58, menu_w - 78, 1, 0x1E293B);

        // App tile helper: 3 columns of tiles
        // Row 1
        // App tile helper: 3 columns of tiles x 4 rows
        struct { const char* name; uint32_t color; } apps[12] = {
            {"Term",     0x38BDF8},
            {"Explr",    0xF59E0B},
            {"Surf",     0x60A5FA},
            {"Store",    0x0284C7},
            {"Notepad",  0xA78BFA},
            {"Clock",    0x34D399},
            {"Paint",    0xEC4899},
            {"Settings", 0x38BDF8},
            {"SysMon",   0xF87171},
            {"Calendar", 0x4ADE80},
            {"Code",     0xFBBF24},
            {"Calc",     0x818CF8},
        };

        int col = 0, row = 0;
        int tile_area_x = menu_x + 14;
        int tile_area_y = menu_y + 70;
        int mt_w = 100, mt_h = 44, mt_gap_x = 12, mt_gap_y = 6;

        for (int i = 0; i < 12; i++) {
            int tx = tile_area_x + col * (mt_w + mt_gap_x);
            int ty = tile_area_y + row * (mt_h + mt_gap_y);
            // Tile bg — gradient rounded rect
            draw_gradient_rounded_rect(tx, ty, mt_w, mt_h, 8, 0x1E293B, 0x0F172A, 1);
            draw_rounded_rect_outline(tx, ty, mt_w, mt_h, 8, 1, 0x334155);
            // Colored top accent
            draw_rounded_rect(tx + 4, ty + 3, mt_w - 8, 2, 0, apps[i].color);
            // App icon circle
            draw_circle(tx + 16, ty + mt_h/2, 6, apps[i].color);
            // Label
            draw_string(tx + 28, ty + mt_h/2 - 4, 0xD1D5DB, apps[i].name);
            col++;
            if (col >= 3) { col = 0; row++; }
        }

        // ── Recommended ────────────────────────────────────────
        int rec_y = tile_area_y + 3 * (mt_h + mt_gap_y) + 6;
        draw_string(menu_x + 16, rec_y, 0xF1F5F9, "Recommended");
        draw_rect(menu_x + 100, rec_y + 4, menu_w - 116, 1, 0x1E293B);

        draw_rounded_rect(menu_x + 14, rec_y + 12, menu_w - 28, 22, 5, 0x111827);
        draw_string(menu_x + 22, rec_y + 17, 0x4ADE80, "/docs/welcome.txt");

        // ── Footer ────────────────────────────────────────────
        int footer_y = menu_y + menu_h - 40;
        draw_rect(menu_x, footer_y, menu_w, 1, 0x1E293B);
        // User avatar circle
        draw_circle(menu_x + 22, footer_y + 20, 12, theme->accent_color);
        draw_string(menu_x + 18, footer_y + 16, 0xFFFFFF, "A");
        draw_string(menu_x + 38, footer_y + 14, 0xF1F5F9, "Administrator");
        draw_string(menu_x + 38, footer_y + 24, 0x6B7280, "root@falkon-os");
        // Power button
        draw_rounded_rect(menu_x + menu_w - 46, footer_y + 10, 34, 22, 6, 0xDC2626);
        draw_string(menu_x + menu_w - 40, footer_y + 15, 0xFFFFFF, "Pwr");
    }

    // ── Quick Settings Flyout ─────────────────────────────────
    if (quick_settings_open) {
        int q_w = 240, q_h = 180;
        int q_x = screen_w - q_w - 14;
        int q_y = taskbar.y_position - q_h - 14;

        draw_rect_alpha(q_x - 6, q_y - 4, q_w + 12, q_h + 12, 0x000000, 55);
        draw_box_blur(q_x, q_y, q_w, q_h, 3);
        draw_rounded_rect(q_x, q_y, q_w, q_h, 12, 0x0F172A);
        draw_rounded_rect_alpha(q_x, q_y, q_w, q_h, 12, 0x1E293B, 190);
        draw_rounded_rect_outline(q_x, q_y, q_w, q_h, 12, 1, theme->accent_color);

        draw_string_shadow(q_x + 14, q_y + 12, 0x38BDF8, 0x000000, "Quick Settings");

        // Toggle tiles (2×2)
        struct { const char* lbl; int on; uint32_t c; } tiles[4] = {
            {"Wi-Fi",  1, 0x0284C7},
            {"Sound",  1, 0x0284C7},
            {"NightLt",0, 0x1E293B},
            {"EXT4",   1, 0x059669},
        };
        for (int i = 0; i < 4; i++) {
            int tx = q_x + 10 + (i % 2) * 115;
            int ty = q_y + 38 + (i / 2) * 50;
            draw_rounded_rect(tx, ty, 105, 38, 8, tiles[i].on ? tiles[i].c : 0x1E293B);
            draw_rounded_rect_outline(tx, ty, 105, 38, 8, 1, 0x334155);
            draw_circle(tx + 16, ty + 19, 7, tiles[i].on ? 0xFFFFFF : 0x4B5563);
            draw_string(tx + 28, ty + 14, 0xF1F5F9, tiles[i].lbl);
        }
        draw_string(q_x + 14, q_y + 150, 0x4B5563, "Falkon-OS Quick Controls v1.0");
    }
}

void taskbar_handle_click(int x, int y) {
    extern int screen_w;
    int menu_w = 360;
    int menu_h = 370;
    int menu_x = (screen_w / 2) - (menu_w / 2);
    int menu_y = taskbar.y_position - menu_h - 12;

    int tray_x = screen_w - 225;

    // Check click on Right System Tray
    if (y >= taskbar.y_position && x >= tray_x) {
        quick_settings_open = !quick_settings_open;
        taskbar.start_menu_open = 0;
        return;
    }

    quick_settings_open = 0;

    // Start Menu Click Handling
    if (taskbar.start_menu_open && 
        x >= menu_x && x < menu_x + menu_w && 
        y >= menu_y && y < menu_y + menu_h) {
        
        int footer_y = menu_y + menu_h - 40;
        if (y >= footer_y && x >= menu_x + menu_w - 50) {
            extern void sys_shutdown(void);
            sys_shutdown();
            return;
        }

        int tile_area_x = menu_x + 14;
        int tile_area_y = menu_y + 70;
        int mt_w = 100, mt_h = 44, mt_gap_x = 12, mt_gap_y = 6;

        for (int i = 0; i < 12; i++) {
            int col = i % 3;
            int row = i / 3;
            int tx = tile_area_x + col * (mt_w + mt_gap_x);
            int ty = tile_area_y + row * (mt_h + mt_gap_y);
            if (x >= tx && x < tx + mt_w && y >= ty && y < ty + mt_h) {
                switch (i) {
                    case 0: { // Term
                        window_t* new_term = wm_create_window(60, 80, 680, 440, "Falkon Bash (fbash)");
                        if (new_term) taskbar_add_button(new_term->id, "Falkon Bash");
                        break;
                    }
                    case 1: file_explorer_open(); break; // Explr
                    case 2: { // Surf
                        extern void browser_open(const char* url);
                        browser_open("file:///docs/welcome.txt");
                        break;
                    }
                    case 3: { // Store
                        extern void store_open(void);
                        store_open();
                        break;
                    }
                    case 4: notepad_open("/docs/welcome.txt"); break; // Notepad
                    case 5: { extern void clock_app_open(void); clock_app_open(); break; } // Clock
                    case 6: { extern void paint_app_open(void); paint_app_open(); break; } // Paint
                    case 7: settings_open(); break; // Settings
                    case 8: sysmon_open(); break;   // SysMon
                    case 9: { extern void calendar_open(void); calendar_open(); break; } // Calendar
                    case 10: { // Code
                        extern void code_editor_open(const char* file_path);
                        code_editor_open("/src/main.c");
                        break;
                    }
                    case 11: { // Calc
                        extern void calc_open(void);
                        calc_open();
                        break;
                    }
                }
                break;
            }
        }

        taskbar.start_menu_open = 0;
        return;
    }

    if (y < taskbar.y_position) {
        taskbar.start_menu_open = 0;
        return;
    }
    
    // Launcher button click
    if (launcher_btn.enabled && 
        x >= launcher_btn.x && 
        x < launcher_btn.x + launcher_btn.width &&
        y >= taskbar.y_position + 4 &&
        y < taskbar.y_position + TASKBAR_HEIGHT - 4) {
        
        taskbar.start_menu_open = !taskbar.start_menu_open;
        return;
    }
    
    taskbar.start_menu_open = 0;

    // Window button click
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        if (y >= taskbar.y_position && x >= btn->x && x < btn->x + btn->width) {
            window_t* win = wm_get_window(btn->window_id);
            if (!win) continue;
            
            if (win->flags & WIN_FLAG_MINIMIZED) {
                wm_restore_window(btn->window_id);
            } else {
                wm_focus_window(btn->window_id);
            }
            return;
        }
    }
}

taskbar_t* taskbar_get_state() {
    return &taskbar;
}
