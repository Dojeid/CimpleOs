#include "taskbar.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "gui/window_manager.h"
#include "gui/terminal.h"
#include "mm/pmm.h"
#include "gui/apps/file_explorer.h"
#include "gui/apps/notepad.h"
#include "gui/apps/sysmon.h"
#include "gui/apps/calc.h"
#include "gui/apps/settings.h"
#include "gui/apps/installer.h"

#define COLOR_TASKBAR_BG      0x0F172A  // Dark Slate Glass
#define COLOR_BUTTON_BG       0x1E293B  // Muted Slate
#define COLOR_BUTTON_ACTIVE   0x0284C7  // Vibrant Cyan Active
#define COLOR_BUTTON_HOVER    0x38BDF8  // Light Cyan Glow
#define COLOR_LAUNCHER_BTN    0x0284C7  // Launcher Cyan Accent

static taskbar_t taskbar;
static launcher_button_t launcher_btn;

void taskbar_init() {
    extern int screen_h;
    
    taskbar.button_count = 0;
    taskbar.y_position = screen_h - TASKBAR_HEIGHT;
    taskbar.start_menu_open = 0;
    
    for (int i = 0; i < MAX_TASKBAR_BUTTONS; i++) {
        taskbar.buttons[i].window_id = -1;
    }
    
    // Modern Start Menu button
    launcher_btn.x = 8;
    launcher_btn.width = 115;
    strcpy(launcher_btn.label, "Falkon Menu");
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
    
    extern int screen_w;
    int available_width = screen_w - (launcher_btn.x + launcher_btn.width + 10 + 20);
    int button_width = available_width / taskbar.button_count;
    
    if (button_width < 80) button_width = 80;
    if (button_width > 130) button_width = 130;
    
    int start_x = launcher_btn.x + launcher_btn.width + 10;
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar.buttons[i].x = start_x + (i * (button_width + 5));
        taskbar.buttons[i].width = button_width;
    }
}

void taskbar_remove_button(int window_id) {
    for (int i = 0; i < taskbar.button_count; i++) {
        if (taskbar.buttons[i].window_id == window_id) {
            for (int j = i; j < taskbar.button_count - 1; j++) {
                taskbar.buttons[j] = taskbar.buttons[j + 1];
            }
            taskbar.button_count--;
            taskbar.buttons[taskbar.button_count].window_id = -1;
            
            extern int screen_w;
            int available_width = screen_w - (launcher_btn.x + launcher_btn.width + 10 + 20);
            int button_width = available_width / (taskbar.button_count > 0 ? taskbar.button_count : 1);
            
            if (button_width < 80) button_width = 80;
            if (button_width > 130) button_width = 130;
            
            int start_x = launcher_btn.x + launcher_btn.width + 10;
            for (int j = 0; j < taskbar.button_count; j++) {
                taskbar.buttons[j].x = start_x + (j * (button_width + 5));
                taskbar.buttons[j].width = button_width;
            }
            return;
        }
    }
}

void taskbar_render() {
    extern int screen_w, screen_h;
    
    // Taskbar main bar background
    draw_rect(0, taskbar.y_position, screen_w, TASKBAR_HEIGHT, COLOR_TASKBAR_BG);
    draw_rect(0, taskbar.y_position, screen_w, 1, 0x334155);
    
    // Render Falkon Menu button
    if (launcher_btn.enabled) {
        uint32_t btn_color = taskbar.start_menu_open ? 0x0369A1 : COLOR_LAUNCHER_BTN;
        draw_rect(launcher_btn.x, taskbar.y_position + 4, launcher_btn.width, TASKBAR_HEIGHT - 8, btn_color);
        draw_string(launcher_btn.x + 10, taskbar.y_position + 9, 0xFFFFFF, launcher_btn.label);
    }
    
    // Render window task buttons
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        window_t* win = wm_get_window(btn->window_id);
        if (!win) continue;
        
        uint32_t btn_color = (win->flags & WIN_FLAG_FOCUSED) ? COLOR_BUTTON_ACTIVE : COLOR_BUTTON_BG;
        draw_rect(btn->x, taskbar.y_position + 4, btn->width, TASKBAR_HEIGHT - 8, btn_color);
        
        char display_label[20];
        if (strlen(btn->label) > 16) {
            strncpy(display_label, btn->label, 13);
            display_label[13] = '.'; display_label[14] = '.'; display_label[15] = '.';
            display_label[16] = '\0';
        } else {
            strcpy(display_label, btn->label);
        }
        
        if (win->flags & WIN_FLAG_MINIMIZED) {
            draw_string(btn->x + 6, taskbar.y_position + 9, 0x94A3B8, display_label);
            draw_string(btn->x + btn->width - 15, taskbar.y_position + 9, 0xF59E0B, "_");
        } else {
            draw_string(btn->x + 6, taskbar.y_position + 9, 0xF1F5F9, display_label);
        }
    }

    // Render Modern Start Menu Popup Overlay if open
    if (taskbar.start_menu_open) {
        int menu_x = 8;
        int menu_y = taskbar.y_position - 245;
        if (menu_y < 80) menu_y = taskbar.y_position - 160;
        int menu_w = 210;
        int menu_h = 240;

        // Glassmorphic Menu Body
        draw_rect(menu_x, menu_y, menu_w, menu_h, 0x0F172A);
        draw_rect(menu_x, menu_y, menu_w, 28, 0x1E293B);
        draw_rect(menu_x, menu_y, menu_w, 2, 0x38BDF8);
        draw_string(menu_x + 12, menu_y + 7, 0x38BDF8, "Falkon Applications");

        // Menu App Launcher Grid
        draw_rect(menu_x + 6, menu_y + 34, menu_w - 12, 26, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 40, 0xF1F5F9, "> Terminal Shell");

        draw_rect(menu_x + 6, menu_y + 64, menu_w - 12, 26, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 70, 0xF1F5F9, "> File Explorer");

        draw_rect(menu_x + 6, menu_y + 94, menu_w - 12, 26, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 100, 0xF1F5F9, "> Notepad Text Editor");

        draw_rect(menu_x + 6, menu_y + 124, menu_w - 12, 26, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 130, 0xF1F5F9, "> System Monitor");

        draw_rect(menu_x + 6, menu_y + 154, menu_w - 12, 26, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 160, 0xF1F5F9, "> Calculator App");

        draw_rect(menu_x + 6, menu_y + 184, menu_w - 12, 26, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 190, 0x38BDF8, "> Settings & Themes");

        draw_rect(menu_x + 6, menu_y + 214, menu_w - 12, 22, 0x1E293B);
        draw_string(menu_x + 12, menu_y + 218, 0x4ADE80, "> OS Installer Wizard");
    }
}

void taskbar_handle_click(int x, int y) {
    int menu_x = 8;
    int menu_y = taskbar.y_position - 245;
    if (menu_y < 80) menu_y = taskbar.y_position - 160;
    int menu_w = 210;
    int menu_h = 240;

    if (taskbar.start_menu_open && 
        x >= menu_x && x < menu_x + menu_w && 
        y >= menu_y && y < menu_y + menu_h) {
        
        // App 1: Terminal
        if (y >= menu_y + 34 && y < menu_y + 60) {
            window_t* new_term = wm_create_window(60, 80, 700, 480, "Terminal");
            if (new_term) {
                new_term->user_data = terminal_get_state();
                taskbar_add_button(new_term->id, "Terminal");
            }
        }
        // App 2: File Explorer
        else if (y >= menu_y + 64 && y < menu_y + 90) {
            file_explorer_open();
        }
        // App 3: Notepad
        else if (y >= menu_y + 94 && y < menu_y + 120) {
            notepad_open("/docs/welcome.txt");
        }
        // App 4: System Monitor
        else if (y >= menu_y + 124 && y < menu_y + 150) {
            sysmon_open();
        }
        // App 5: Calculator
        else if (y >= menu_y + 154 && y < menu_y + 180) {
            calc_open();
        }
        // App 6: Settings
        else if (y >= menu_y + 184 && y < menu_y + 210) {
            settings_open();
        }
        // App 7: OS Installer Wizard
        else if (y >= menu_y + 214 && y < menu_y + 236) {
            installer_open();
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
