#include "gui/apps/calendar.h"
#include "drivers/rtc.h"
#include "drivers/video/graphics.h"
#include "gui/window_manager.h"
#include "lib/printf.h"
#include "lib/string.h"
#include "gui/taskbar.h"

static int cal_month = 0;
static int cal_year = 0;
static int current_day = 0;

static const char* month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static int days_in_month(int month, int year) {
    if (month == 1) { // February
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
        return 28;
    }
    if (month == 3 || month == 5 || month == 8 || month == 10) return 30;
    return 31;
}

// Zeller's congruence to get day of week (0 = Sunday, 1 = Monday, ...)
static int get_dow(int d, int m, int y) {
    m += 1; // 1-indexed for Zeller
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int k = y % 100;
    int j = y / 100;
    int f = d + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) + (5 * j);
    int dow = (f + 5) % 7;
    return dow;
}

static void cal_render(window_t* win) {
    if (!win) return;
    
    // Background
    draw_rect(win->x, win->y + 32, win->width, win->height - 32, 0xFFFFFF);
    
    // Header background
    draw_rect(win->x, win->y + 32, win->width, 40, 0x3B82F6);
    
    char title[64];
    snprintf(title, 64, "<   %s %d   >", month_names[cal_month], cal_year);
    draw_string(win->x + win->width / 2 - 60, win->y + 44, 0xFFFFFF, title);
    
    // Days of week header
    const char* dows[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    int start_x = win->x + 10;
    int start_y = win->y + 80;
    int cell_w = 40;
    
    for (int i = 0; i < 7; i++) {
        draw_string(start_x + i * cell_w + 10, start_y, 0x6B7280, dows[i]);
    }
    
    // Calendar grid
    int first_dow = get_dow(1, cal_month, cal_year);
    int days = days_in_month(cal_month, cal_year);
    
    int row = 0;
    int col = first_dow;
    
    for (int d = 1; d <= days; d++) {
        int x = start_x + col * cell_w;
        int y = start_y + 24 + row * 32;
        
        // Highlight current day if it's the current month/year
        rtc_time_t curr_time;
        rtc_read(&curr_time);
        
        uint32_t text_color = 0x111827;
        if (cal_month == curr_time.month - 1 && cal_year == curr_time.year && d == curr_time.day) {
            draw_rounded_rect(x + 4, y - 4, 28, 28, 14, 0x3B82F6);
            text_color = 0xFFFFFF;
        }
        
        char day_str[8];
        snprintf(day_str, 8, "%d", d);
        int x_offset = (d < 10) ? 14 : 10;
        draw_string(x + x_offset, y + 2, text_color, day_str);
        
        col++;
        if (col > 6) {
            col = 0;
            row++;
        }
    }
}

static void cal_click(window_t* win, int rel_x, int rel_y) {
    if (rel_y >= 32 && rel_y <= 72) {
        if (rel_x < win->width / 2) {
            // Previous month
            cal_month--;
            if (cal_month < 0) {
                cal_month = 11;
                cal_year--;
            }
        } else {
            // Next month
            cal_month++;
            if (cal_month > 11) {
                cal_month = 0;
                cal_year++;
            }
        }
        if (win->render_content) win->render_content(win);
    }
}

void calendar_open(void) {
    rtc_time_t time;
    rtc_read(&time);
    cal_month = time.month - 1;
    cal_year = time.year;
    current_day = time.day;
    
    window_t* win = wm_create_window(200, 150, 320, 280, "Calendar");
    if (win) {
        win->render_content = cal_render;
        win->on_click = cal_click;
        taskbar_add_button(win->id, "Calendar");
        cal_render(win);
    }
}
