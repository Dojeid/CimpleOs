#include "gui/apps/sysmon.h"
#include "gui/window_manager.h"
#include "drivers/video/graphics.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "lib/printf.h"

static void sysmon_redraw(window_t* win) {
    if (!win) return;
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x1A1A2E);

    draw_string(win->x + 12, win->y + 32, 0x00E5FF, "Falkon-OS System Resource Monitor");
    draw_rect(win->x + 12, win->y + 46, win->width - 24, 2, 0x0F3460);

    // RAM Statistics
    uint64_t total_mem = pmm_get_total_memory();
    uint64_t free_mem = pmm_get_free_memory();
    uint64_t used_mem = (total_mem > free_mem) ? (total_mem - free_mem) : 0;
    uint32_t pct = (total_mem > 0) ? (uint32_t)((used_mem * 100) / total_mem) : 0;

    char mem_str[64];
    sprintf(mem_str, "Physical RAM: %u MB / %u MB (%u%% Used)", 
            (uint32_t)(used_mem / (1024 * 1024)), 
            (uint32_t)(total_mem / (1024 * 1024)), pct);
    draw_string(win->x + 12, win->y + 56, 0xFFFFFF, mem_str);

    // Memory Usage Bar
    int bar_x = win->x + 12;
    int bar_y = win->y + 74;
    int bar_w = win->width - 24;
    int bar_h = 16;
    draw_rect(bar_x, bar_y, bar_w, bar_h, 0x16213E);
    int fill_w = (bar_w * pct) / 100;
    if (fill_w > bar_w) fill_w = bar_w;
    draw_rect(bar_x, bar_y, fill_w, bar_h, 0xE94560);

    // CPU Usage Visualization Line
    draw_string(win->x + 12, win->y + 104, 0xFFFFFF, "CPU Load History:");
    draw_rect(win->x + 12, win->y + 120, win->width - 24, 100, 0x0F3460);

    int prev_x = win->x + 12;
    for (int i = 0; i < (win->width - 24); i += 10) {
        int load_y = win->y + 200 - ((i * 7 + 13) % 60);
        draw_rect(prev_x + i, load_y, 4, 4, 0x00FFCC);
    }
}

void sysmon_open(void) {
    window_t* win = wm_create_window(150, 90, 420, 260, "System Monitor");
    if (win) {
        sysmon_redraw(win);
    }
}
