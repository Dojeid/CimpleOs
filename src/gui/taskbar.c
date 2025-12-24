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
    
    for (int i = 0; i < MAX_TASKBAR_BUTTONS; i++) {
        taskbar.buttons[i].window_id = -1;
    }
    
    // UX FIX: Add permanent Terminal launcher button
    launcher_btn.x = 10;
    launcher_btn.width = 100;
    strcpy(launcher_btn.label, "Terminal");
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
    
    // BUG FIX #5: Recalculate ALL button positions dynamically to prevent overflow
    extern int screen_w;
    int available_width = screen_w - 260;  // Reserve space for launcher (120px) + system tray (140px)
    int button_width = available_width / taskbar.button_count;
    
    // Minimum button width of 80px, maximum 120px
    if (button_width < 80) button_width = 80;
    if (button_width > 120) button_width = 120;
    
    // Reposition all buttons (start after launcher button)
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
            
            // BUG FIX #5: Recalculate positions after removal
            extern int screen_w;
            int available_width = screen_w - 160;
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
    
    // UX FIX: Render launcher button (permanent Terminal launcher)
    if (launcher_btn.enabled) {
        draw_rect(launcher_btn.x, taskbar.y_position + 5, 
                  launcher_btn.width, TASKBAR_HEIGHT - 10, COLOR_LAUNCHER_BTN);
        draw_string(launcher_btn.x + 10, taskbar.y_position + 10, 0xFFFFFF, launcher_btn.label);
    }
    
    // Render window buttons
    window_manager_t* wm = wm_get_state();
    
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        window_t* win = wm_get_window(btn->window_id);
        
        if (!win) continue;
        
        // Determine button color
        uint32_t btn_color = COLOR_BUTTON_BG;
        if (win->flags & WIN_FLAG_FOCUSED) {
            btn_color = COLOR_BUTTON_ACTIVE;
        }
        
        // Draw button
        draw_rect(btn->x, taskbar.y_position + 5, btn->width, TASKBAR_HEIGHT - 10, btn_color);
        
        // Draw label (truncate if needed)
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
        
        // Show minimize indicator
        if (win->flags & WIN_FLAG_MINIMIZED) {
            draw_string(btn->x + 5, taskbar.y_position + 10, 0xBDC3C7, display_label);
            draw_string(btn->x + btn->width - 15, taskbar.y_position + 10, 0xF39C12, "_");
        } else {
            draw_string(btn->x + 5, taskbar.y_position + 10, 0xECF0F1, display_label);
        }
    }
    
    // UX: RAM moved to top bar - taskbar now cleaner
    // System tray area reserved for future use (clock, notifications, etc.)
}

void taskbar_handle_click(int x, int y) {
    if (y < taskbar.y_position) return;
    
    // UX FIX: Check if clicking launcher button first
    if (launcher_btn.enabled && 
        x >= launcher_btn.x && 
        x < launcher_btn.x + launcher_btn.width &&
        y >= taskbar.y_position + 5 &&
        y < taskbar.y_position + TASKBAR_HEIGHT - 5) {
        
        // Launch new terminal!
        extern window_t* wm_create_window(int, int, int, int, const char*);
        extern terminal_instance_t* terminal_create_instance();
        extern void terminal_instance_print(terminal_instance_t*, const char*);
        
        // Simple position offset (no rand in kernel)
        static int term_counter = 0;
        term_counter++;
        int offset_x = 50 + ((term_counter * 30) % 200);
        int offset_y = 80 + ((term_counter * 25) % 150);
        
        // FEATURE 1: Create fully functional independent terminal
        window_t* new_term = wm_create_window(offset_x, offset_y, 700, 480, "Terminal");
        if (new_term) {
            // Create independent terminal instance
            new_term->user_data = terminal_create_instance();
            new_term->render_content = NULL;
            taskbar_add_button(new_term->id, "Terminal");
            
            // Print welcome to this specific instance
            terminal_instance_t* term = (terminal_instance_t*)new_term->user_data;
            terminal_instance_print(term, "New independent terminal!");
            terminal_instance_print(term, "This has its own buffer and history.");
            terminal_instance_print(term, "");
        }
        return;
    }
    
    // Check if clicking a window button
    for (int i = 0; i < taskbar.button_count; i++) {
        taskbar_button_t* btn = &taskbar.buttons[i];
        
        if (x >= btn->x && x < btn->x + btn->width) {
            window_t* win = wm_get_window(btn->window_id);
            if (!win) continue;
            
            // Toggle minimize/restore
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
