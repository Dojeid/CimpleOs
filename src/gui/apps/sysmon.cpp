// sysmon.cpp — System Monitor App (C++)
// Core OS kernel headers included via extern "C"
extern "C" {
#include "gui/apps/sysmon.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "fs/ext4.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "kernel/timer.h"
#include "kernel/process.h"
#include "lib/printf.h"
#include "lib/string.h"
}

static void sysmon_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 10;
    int y = win->y + 32;

    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);
    draw_string(x, y, 0x38BDF8, "Falkon-OS Hardware & System Resource Monitor");
    draw_rect(x, y + 16, win->width - 20, 2, 0x334155);

    // 1. Physical Memory Statistics
    uint64_t total_mem = pmm_get_total_memory();
    uint64_t free_mem  = pmm_get_free_memory();
    uint64_t used_mem  = (total_mem > free_mem) ? (total_mem - free_mem) : 0;
    uint32_t pct = (total_mem > 0) ? (uint32_t)((used_mem * 100) / total_mem) : 0;

    char mem_str[128];
    sprintf(mem_str, "Physical RAM: %u MB / %u MB (%u%% Used)",
            (uint32_t)(used_mem / (1024*1024)),
            (uint32_t)(total_mem / (1024*1024)), pct);
    draw_string(x, y + 26, 0xF1F5F9, mem_str);

    // RAM bar
    int bar_w = win->width - 20;
    draw_rect(x, y + 44, bar_w, 16, 0x1E293B);
    draw_rect(x, y + 44, bar_w, 1, 0x334155);
    int fill_w = (bar_w * (int)pct) / 100;
    if (fill_w > bar_w) fill_w = bar_w;
    draw_rect(x, y + 44, fill_w, 16, 0x38BDF8);

    // 2. CPU / Uptime
    extern volatile uint32_t timer_ticks;
    uint32_t uptime_sec = timer_ticks / 100;
    char cpu_str[128];
    sprintf(cpu_str, "CPU Scheduler: Uptime %us (PIT IRQ0 @ 100Hz)", uptime_sec);
    draw_string(x, y + 70, 0x38BDF8, cpu_str);

    // Waveform graph
    int graph_w = win->width - 20, graph_h = 75;
    draw_rect(x, y + 88, graph_w, graph_h, 0x1E293B);
    draw_rect(x, y + 88, graph_w, 1, 0x334155);
    for (int i = 0; i < graph_w - 4; i += 6) {
        uint32_t sample = (timer_ticks * 3 + (uint32_t)(i * 7)) % (uint32_t)(graph_h - 10);
        int py = y + 88 + graph_h - 5 - (int)sample;
        draw_rect(x + 2 + i, py, 4, 3, 0xFF007F);
    }

// 3. Disk
draw_string(x, y + 172, 0x38BDF8, "Disk Storage & EXT4 Volume Status:");
    ata_drive_t* drv = ata_get_drive(0);
    char ata_info[128];
    if (drv && drv->present) {
        sprintf(ata_info, "ATA Disk: %s (%u MB)",
                drv->model,
                (drv->total_sectors * 512) / (1024*1024));
        draw_string(x + 10, y + 190, 0x4ADE80, ata_info);
    } else {
        strcpy(ata_info, "ATA Disk: Virtual Storage (0 MB)");
        draw_string(x + 10, y + 190, 0xFF0000, ata_info);
    }

    char ext4_info[128];
    if (ext4_is_mounted()) {
        sprintf(ext4_info, "Mount: / (EXT4 Mounted, Magic: 0xEF53)");
        draw_string(x + 10, y + 206, 0x4ADE80, ext4_info);
    } else {
        sprintf(ext4_info, "Mount: / (EXT4 Not Mounted)");
        draw_string(x + 10, y + 206, 0xFF0000, ext4_info);
    }
}

extern "C" void sysmon_open(void) {
    window_t* win = wm_create_window(140, 80, 440, 275, "Hardware System Monitor");
    if (win) {
        win->render_content = sysmon_redraw;
        taskbar_add_button(win->id, "SysMon");
        sysmon_redraw(win);
    }
}
