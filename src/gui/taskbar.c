#include "taskbar.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "gui/window_manager.h"
#include "gui/terminal.h"
#include "mm/pmm.h"

#define COLOR_TASKBAR_BG 0x34495E
#define COLOR_BUTTON_BG 0x2C3E50
#define COLOR_BUTTON_ACTIVE 0x3498DB
#define COLOR_BUTTON_HOVER 0x5DADE2
#define COLOR_LAUNCHER_BTN 0x27AE60  // Green for launcher

static taskbar_t taskbar;
static launcher_button_t launcher_btn;  // Terminal launcher

void taskbar_init() {
    extern int screen_h;
    
    taskbar.button_count = 0;
    taskbar.y_position = screen_h - TASKBAR_HEIGHT;
    taskbar.start_menu_open = 0;
    
    for (int i = 0; i < MAX_TASKBAR_BUTTONS; i++) {
        taskbar.buttons[i].window_id = -1;
    }
    
    // Add Start Menu launcher button
    launcher_btn.x = 10;
    launcher_btn.width = 110;
    strcpy(launcher_btn.label, "Falkon Menu");
    launcher_btn.enabled = 1;
}

void taskbar_add_button(int window_id, const char* label) {
    if (taskbar.button_count >= MAX_TASKBAR_BUTTONS) return;
    
    // Check if button already exists
    for (int i = 0; i < taskbar.button_count; i++) {
        if (taskbar.buttons[i].window_id == window_id) {
            return;  // Already have a button for this window
        }
    }
    
    taskbar_button_t* btn = &taskbar.buttons[taskbar.button_count];
    btn->window_id = window_id;
    strncpy(btn->label, label, 31);
    btn->label[31] = '\0';
    
    taskbar.button_count++;
    
    extern int screen_w;
    int available_width = screen_w - 270;
    int button_width = available_width / taskbar.button_count;
    
    if (button_width < 80) button_width = 80;
    if (button_width > 120) button_width = 120;
    
    int start_x = launcher_btn.x + launcher_btn.width + 10;
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar.buttons[i].x = start_x + (i * (button_width + 5));
        taskbar.buttons[i].width = button_width;
    }
}

void taskbar_remove_button(int window_id) {
    for (int i = 0; i < taskbar.button_count; i++) {
        if (taskbar.buttons[i].window_id == window_id) {
            // Shift remaining buttons
            for (int j = i; j < taskbar.button_count - 1; j++) {
                taskbar.buttons[j] = taskbar.buttons[j + 1];
            }
            taskbar.button_count--;
            taskbar.buttons[taskbar.button_count].window_id = -1;
            
            extern int screen_w;
            int available_width = screen_w - 170;
            int button_width = available_width / (taskbar.button_count > 0 ? taskbar.button_count : 1);
            
            if (button_width < 80) button_width = 80;
            if (button_width > 120) button_width = 120;
            
            for (int j = 0; j < taskbar.button_count; j++) {
                taskbar.buttons[j].x = 10 + (j * (button_width + 5));
                taskbar.buttons[j].width = button_width;
            }
            
            return;
        }
    }
}

void taskbar_render() {
    extern int screen_w, screen_h;
    
    // Taskbar background
    draw_rect(0, taskbar.y_position, screen_w, TASKBAR_HEIGHT, COLOR_TASKBAR_BG);
    
    // Render launcher button (Falkon Menu)
    if (launcher_btn.enabled) {
        uint32_t btn_color = taskbar.start_menu_open ? COLOR_BUTTON_ACTIVE : COLOR_LAUNCHER_BTN;
        draw_rect(launcher_btn.x, taskbar.y_position + 4, 
                  launcher_btn.width, TASKBAR_HEIGHT - 8, btn_color);
        draw_string(launcher_btn.x + 8, taskbar.y_position + 9, 0xFFFFFF, launcher_btn.label);
    }
    
    // Render window buttons
    
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        window_t* win = wm_get_window(btn->window_id);
        
        if (!win) continue;
        
        uint32_t btn_color = COLOR_BUTTON_BG;
        if (win->flags & WIN_FLAG_FOCUSED) {
            btn_color = COLOR_BUTTON_ACTIVE;
        }
        
        draw_rect(btn->x, taskbar.y_position + 4, btn->width, TASKBAR_HEIGHT - 8, btn_color);
        
        char display_label[20];
        if (strlen(btn->label) > 18) {
            strncpy(display_label, btn->label, 15);
            display_label[15] = '.';
            display_label[16] = '.';
            display_label[17] = '.';
            display_label[18] = '\0';
        } else {
            strcpy(display_label, btn->label);
        }
        
        if (win->flags & WIN_FLAG_MINIMIZED) {
            draw_string(btn->x + 5, taskbar.y_position + 9, 0xBDC3C7, display_label);
            draw_string(btn->x + btn->width - 15, taskbar.y_position + 9, 0xF39C12, "_");
        } else {
            draw_string(btn->x + 5, taskbar.y_position + 9, 0xECF0F1, display_label);
        }
    }

    // Render Start Menu Popup Overlay if open
    if (taskbar.start_menu_open) {
        int menu_x = 10;
        int menu_y = taskbar.y_position - 180;
        int menu_w = 180;
        int menu_h = 175;

        // Outer Shadow & Menu Background
        draw_rect(menu_x + 3, menu_y + 3, menu_w, menu_h, 0x111111);
        draw_rect(menu_x, menu_y, menu_w, menu_h, 0x1F2937);
        draw_rect(menu_x, menu_y, menu_w, 24, 0x374151);
        draw_string(menu_x + 10, menu_y + 5, 0x38BDF8, "Falkon Applications");

        // Menu Items
        draw_rect(menu_x + 5, menu_y + 30, menu_w - 10, 24, 0x374151);
        draw_string(menu_x + 12, menu_y + 35, 0xFFFFFF, "> Terminal");

        draw_rect(menu_x + 5, menu_y + 58, menu_w - 10, 24, 0x374151);
        draw_string(menu_x + 12, menu_y + 63, 0xFFFFFF, "> File Explorer");

        draw_rect(menu_x + 5, menu_y + 86, menu_w - 10, 24, 0x374151);
        draw_string(menu_x + 12, menu_y + 91, 0xFFFFFF, "> Notepad Text");

        draw_rect(menu_x + 5, menu_y + 114, menu_w - 10, 24, 0x374151);
        draw_string(menu_x + 12, menu_y + 119, 0xFFFFFF, "> System Monitor");

        draw_rect(menu_x + 5, menu_y + 142, menu_w - 10, 24, 0x374151);
        draw_string(menu_x + 12, menu_y + 147, 0xFFFFFF, "> Calculator");
    }
}

void taskbar_handle_click(int x, int y) {
    int menu_x = 10;
    int menu_y = taskbar.y_position - 180;
    int menu_w = 180;
    int menu_h = 175;

    // Check if Start Menu is open and clicked inside Start Menu
    if (taskbar.start_menu_open && 
        x >= menu_x && x < menu_x + menu_w && 
        y >= menu_y && y < menu_y + menu_h) {
        
        // Item 1: Terminal
        if (y >= menu_y + 30 && y < menu_y + 54) {
            extern window_t* wm_create_window(int, int, int, int, const char*);
            extern terminal_instance_t* terminal_create_instance();
            extern void terminal_instance_print(terminal_instance_t*, const char*);
            
            window_t* new_term = wm_create_window(50, 80, 700, 480, "Terminal");
            if (new_term) {
                new_term->user_data = terminal_create_instance();
                new_term->render_content = NULL;
                taskbar_add_button(new_term->id, "Terminal");
                terminal_instance_t* term = (terminal_instance_t*)new_term->user_data;
                terminal_instance_print(term, "Falkon-OS Terminal Session");
                terminal_instance_print(term, "Type 'help' or 'ls' for commands.");
                terminal_instance_print(term, "");
            }
        }
        // Item 2: File Explorer
        else if (y >= menu_y + 58 && y < menu_y + 82) {
            #include "gui/apps/file_explorer.h"
            file_explorer_open();
        }
        // Item 3: Notepad
        else if (y >= menu_y + 86 && y < menu_y + 110) {
            #include "gui/apps/notepad.h"
            notepad_open("/docs/welcome.txt");
        }
        // Item 4: System Monitor
        else if (y >= menu_y + 114 && y < menu_y + 138) {
            #include "gui/apps/sysmon.h"
            sysmon_open();
        }
        // Item 5: Calculator
        else if (y >= menu_y + 142 && y < menu_y + 166) {
            #include "gui/apps/calc.h"
            calc_open();
        }

        taskbar.start_menu_open = 0;
        return;
    }

    if (y < taskbar.y_position) {
        taskbar.start_menu_open = 0;
        return;
    }
    
    // Check if clicking launcher button
    if (launcher_btn.enabled && 
        x >= launcher_btn.x && 
        x < launcher_btn.x + launcher_btn.width &&
        y >= taskbar.y_position + 4 &&
        y < taskbar.y_position + TASKBAR_HEIGHT - 4) {
        
        taskbar.start_menu_open = !taskbar.start_menu_open;
        return;
    }
    
    taskbar.start_menu_open = 0;

    // Check if clicking a window button
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
