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
    
    // Windows 11 Acrylic Taskbar background bar
    draw_rect(0, taskbar.y_position, screen_w, TASKBAR_HEIGHT, theme->taskbar_color);
    draw_rect(0, taskbar.y_position, screen_w, 1, theme->accent_color);

    // Calculate Windows 11 CENTERED Dock Positioning
    int button_w = 95;
    int total_dock_width = launcher_btn.width + (taskbar.button_count * (button_w + 6));
    int start_x = (screen_w / 2) - (total_dock_width / 2);
    if (start_x < 10) start_x = 10;

    launcher_btn.x = start_x;

    // Render Windows 11 Centered Start Button
    if (launcher_btn.enabled) {
        uint32_t btn_color = taskbar.start_menu_open ? 0x0369A1 : theme->accent_color;
        draw_rect(launcher_btn.x, taskbar.y_position + 4, launcher_btn.width, TASKBAR_HEIGHT - 8, btn_color);
        draw_string(launcher_btn.x + 8, taskbar.y_position + 9, 0xFFFFFF, launcher_btn.label);
    }

    // Render Windows 11 Centered App Icons
    int cur_x = start_x + launcher_btn.width + 6;
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        window_t* win = wm_get_window(btn->window_id);
        if (!win) continue;

        btn->x = cur_x;
        btn->width = button_w;
        
        uint32_t btn_color = (win->flags & WIN_FLAG_FOCUSED) ? COLOR_BUTTON_ACTIVE : COLOR_BUTTON_BG;
        draw_rect(btn->x, taskbar.y_position + 4, btn->width, TASKBAR_HEIGHT - 8, btn_color);

        // Windows 11 Active App Bottom Accent Bar
        if (win->flags & WIN_FLAG_FOCUSED) {
            draw_rect(btn->x + 10, taskbar.y_position + TASKBAR_HEIGHT - 3, btn->width - 20, 2, theme->accent_color);
        }

        char display_label[20];
        if (strlen(btn->label) > 11) {
            strncpy(display_label, btn->label, 8);
            display_label[8] = '.'; display_label[9] = '.'; display_label[10] = '\0';
        } else {
            strcpy(display_label, btn->label);
        }
        
        uint32_t text_col = (win->flags & WIN_FLAG_MINIMIZED) ? 0x94A3B8 : 0xF1F5F9;
        draw_string(btn->x + 6, taskbar.y_position + 9, text_col, display_label);

        cur_x += button_w + 6;
    }

    // Windows 11 Right System Tray (Volume, Wi-Fi, Battery, Clock)
    int tray_x = screen_w - 225;
    int tray_y = taskbar.y_position + 4;
    draw_rect(tray_x, tray_y, 220, TASKBAR_HEIGHT - 8, 0x1E293B);
    draw_rect(tray_x, tray_y, 220, 1, 0x334155);

    // Status Badges
    draw_string(tray_x + 6, tray_y + 5, 0x4ADE80, "VOL:100%");
    draw_string(tray_x + 75, tray_y + 5, 0x38BDF8, "WiFi");
    
    extern volatile uint32_t timer_ticks;
    uint32_t sec = timer_ticks / 100;
    uint32_t hrs = (sec / 3600) % 24;
    uint32_t mins = (sec % 3600) / 60;
    char clock_str[16];
    sprintf(clock_str, "%02u:%02u", hrs, mins);
    draw_string(tray_x + 120, tray_y + 5, 0xFFFFFF, clock_str);
    draw_string(tray_x + 175, tray_y + 5, 0xF59E0B, "[3]");

    // Render Windows 11 Centered Start Menu Popup
    if (taskbar.start_menu_open) {
        int menu_w = 340;
        int menu_h = 330;
        int menu_x = (screen_w / 2) - (menu_w / 2);
        int menu_y = taskbar.y_position - menu_h - 10;

        // Outer Glassmorphic Start Panel
        draw_rect(menu_x, menu_y, menu_w, menu_h, 0x0F172A);
        draw_rect(menu_x, menu_y, menu_w, 36, 0x1E293B);
        draw_rect(menu_x, menu_y, menu_w, 2, theme->accent_color);

        // Windows 11 Search Input Bar
        draw_rect(menu_x + 12, menu_y + 8, menu_w - 24, 22, 0x0B0F19);
        draw_string(menu_x + 20, menu_y + 12, 0x94A3B8, "[?] Type here to search apps, files...");

        // Pinned Section Header
        draw_string(menu_x + 14, menu_y + 44, theme->accent_color, "Pinned Applications");

        // 2-Column App Grid Layout
        draw_rect(menu_x + 12, menu_y + 62, 152, 32, 0x1E293B);
        draw_string(menu_x + 20, menu_y + 70, 0xF1F5F9, "> Terminal Shell");

        draw_rect(menu_x + 176, menu_y + 62, 152, 32, 0x1E293B);
        draw_string(menu_x + 184, menu_y + 70, 0xF1F5F9, "> File Explorer");

        draw_rect(menu_x + 12, menu_y + 100, 152, 32, 0x1E293B);
        draw_string(menu_x + 20, menu_y + 108, 0xF1F5F9, "> Text Editor");

        draw_rect(menu_x + 176, menu_y + 100, 152, 32, 0x1E293B);
        draw_string(menu_x + 184, menu_y + 108, 0x4ADE80, "> OS Installer");

        draw_rect(menu_x + 12, menu_y + 138, 152, 32, 0x1E293B);
        draw_string(menu_x + 20, menu_y + 146, 0x38BDF8, "> Control Panel");

        draw_rect(menu_x + 176, menu_y + 138, 152, 32, 0x1E293B);
        draw_string(menu_x + 184, menu_y + 146, 0xF1F5F9, "> Task Manager");

        // Recommended System Files Section
        draw_string(menu_x + 14, menu_y + 182, 0x94A3B8, "Recommended Files");
        draw_string(menu_x + 20, menu_y + 202, 0xE2E8F0, "* /docs/welcome.txt (Text)");
        draw_string(menu_x + 20, menu_y + 220, 0xE2E8F0, "* /sys/os_info.cfg (Config)");
        draw_string(menu_x + 20, menu_y + 238, 0x4ADE80, "* /dev/sda (EXT4 Volume)");

        // Windows 11 User Profile & Power Footer
        draw_rect(menu_x, menu_y + menu_h - 36, menu_w, 36, 0x1E293B);
        draw_rect(menu_x, menu_y + menu_h - 36, menu_w, 1, 0x334155);
        draw_string(menu_x + 14, menu_y + menu_h - 24, 0xFFFFFF, "Administrator (root)");

        draw_rect(menu_x + menu_w - 85, menu_y + menu_h - 30, 75, 24, 0xDC2626);
        draw_string(menu_x + menu_w - 78, menu_y + menu_h - 24, 0xFFFFFF, "[x] Power");
    }

    // Windows 11 Quick Settings Flyout
    if (quick_settings_open) {
        int q_w = 220;
        int q_h = 160;
        int q_x = screen_w - q_w - 10;
        int q_y = taskbar.y_position - q_h - 10;

        draw_rect(q_x, q_y, q_w, q_h, 0x0F172A);
        draw_rect(q_x, q_y, q_w, 28, 0x1E293B);
        draw_rect(q_x, q_y, q_w, 2, theme->accent_color);
        draw_string(q_x + 10, q_y + 6, 0x38BDF8, "Quick Controls");

        draw_rect(q_x + 10, q_y + 36, 95, 26, 0x0284C7);
        draw_string(q_x + 16, q_y + 42, 0xFFFFFF, "Wi-Fi: ON");

        draw_rect(q_x + 115, q_y + 36, 95, 26, 0x0284C7);
        draw_string(q_x + 121, q_y + 42, 0xFFFFFF, "Audio: 100%");

        draw_rect(q_x + 10, q_y + 70, 95, 26, 0x1E293B);
        draw_string(q_x + 16, q_y + 76, 0x94A3B8, "Night Light");

        draw_rect(q_x + 115, q_y + 70, 95, 26, 0x10B981);
        draw_string(q_x + 121, q_y + 76, 0xFFFFFF, "EXT4: OK");

        draw_string(q_x + 10, q_y + 110, 0x94A3B8, "Falkon-OS Windows 11 UI Engine");
    }
}

void taskbar_handle_click(int x, int y) {
    extern int screen_w;
    int menu_w = 340;
    int menu_h = 330;
    int menu_x = (screen_w / 2) - (menu_w / 2);
    int menu_y = taskbar.y_position - menu_h - 10;

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
        
        // App Grid Item Clicks
        if (y >= menu_y + 62 && y < menu_y + 94) {
            if (x >= menu_x + 12 && x < menu_x + 164) {
                window_t* new_term = wm_create_window(60, 80, 700, 480, "Terminal");
                if (new_term) taskbar_add_button(new_term->id, "Terminal");
            } else if (x >= menu_x + 176 && x < menu_x + 328) {
                file_explorer_open();
            }
        }
        else if (y >= menu_y + 100 && y < menu_y + 132) {
            if (x >= menu_x + 12 && x < menu_x + 164) {
                notepad_open("/docs/welcome.txt");
            } else if (x >= menu_x + 176 && x < menu_x + 328) {
                installer_open();
            }
        }
        else if (y >= menu_y + 138 && y < menu_y + 170) {
            if (x >= menu_x + 12 && x < menu_x + 164) {
                settings_open();
            } else if (x >= menu_x + 176 && x < menu_x + 328) {
                sysmon_open();
            }
        }
        else if (y >= menu_y + menu_h - 30 && x >= menu_x + menu_w - 85) {
            outb(0x64, 0xFE); // Keyboard controller reboot
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
        if (x >= btn->x && x < btn->x + btn->width) {
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
