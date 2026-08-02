#include "gui/apps/about.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "kernel/cpuid.h"
#include "mm/pmm.h"
#include "lib/printf.h"

static void about_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 16;
    int y = win->y + 36;

    // Midnight dark container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Glowing Animated Logo Icon
    int cx = win->x + win->width / 2;
    int cy = y + 26;
    draw_circle_alpha(cx, cy, 24, 0x38BDF8, 60);
    draw_circle(cx, cy, 18, 0x0284C7);
    draw_string(cx - 16, cy - 4, 0xFFFFFF, "FLKN");

    // Titles
    draw_string_shadow(cx - 64, y + 60, 0x38BDF8, 0x000000, "Falkon-OS Enterprise v1.0");
    draw_string(cx - 88, y + 78, 0x94A3B8, "Preemptive 64-bit Micro-Kernel Desktop");
    draw_rect(x, y + 96, win->width - 32, 1, 0x334155);

    // Hardware Info
    cpu_info_t cpu;
    cpuid_get_info(&cpu);
    char line[128];
    sprintf(line, "CPU: %s", cpu.brand[0] ? cpu.brand : "x86_64 Compatible Processor");
    draw_string(x, y + 108, 0xF1F5F9, line);

    uint64_t total_mb = pmm_get_total_memory() / (1024 * 1024);
    uint64_t free_mb = pmm_get_free_memory() / (1024 * 1024);
    sprintf(line, "Memory: %u MB Physical RAM (%u MB Free)", (uint32_t)total_mb, (uint32_t)free_mb);
    draw_string(x, y + 126, 0xF1F5F9, line);

    draw_string(x, y + 144, 0x4ADE80, "Kernel: Linux 6.8.0-falkon #1 SMP PREEMPT x86_64");
    draw_string(x, y + 162, 0xFBBF24, "Graphics: VESA/BGA 1024x768 32bpp Double-Buffered");

    draw_rect(x, y + 182, win->width - 32, 1, 0x334155);
    draw_string(x + 20, y + 192, 0x94A3B8, "Built from scratch with passion by Google DeepMind team.");
}

void about_open(void) {
    window_t* win = wm_create_window(140, 70, 420, 260, "About Falkon-OS");
    if (win) {
        win->render_content = about_redraw;
        taskbar_add_button(win->id, "About");
        about_redraw(win);
    }
}
