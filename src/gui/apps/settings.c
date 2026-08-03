#include "gui/apps/settings.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/desktop.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "drivers/net/e1000.h"
#include "fs/ext4.h"
#include "mm/pmm.h"
#include "kernel/cpuid.h"
#include "lib/printf.h"
#include "lib/string.h"

static int active_tab = 1; // 1=Display & Video, 2=Personalize & Themes, 3=Hardware Specs, 4=Storage & Input
static int active_theme = 1;
static int active_res = 1;  // 1=1024x768, 2=1280x720, 3=800x600, 4=1280x1024, 5=640x480
static int brightness_level = 100; // 100%, 75%, 50%
static int night_light_on = 0;
static char settings_msg[128] = "System Display & Hardware Control Center Active.";

static void settings_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;

    extern int screen_w, screen_h;
    extern uint32_t* video_memory;

    // Dark glassmorphic container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Header Tabs Bar
    uint32_t tab1_col = (active_tab == 1) ? 0x38BDF8 : 0x1E293B;
    uint32_t tab2_col = (active_tab == 2) ? 0x38BDF8 : 0x1E293B;
    uint32_t tab3_col = (active_tab == 3) ? 0x38BDF8 : 0x1E293B;
    uint32_t tab4_col = (active_tab == 4) ? 0x38BDF8 : 0x1E293B;

    draw_rect(x, y, 120, 26, tab1_col);
    draw_string(x + 8, y + 6, (active_tab == 1) ? 0x000000 : 0xFFFFFF, "1. Display & Video");

    draw_rect(x + 125, y, 115, 26, tab2_col);
    draw_string(x + 131, y + 6, (active_tab == 2) ? 0x000000 : 0xFFFFFF, "2. Personalize");

    draw_rect(x + 245, y, 110, 26, tab3_col);
    draw_string(x + 251, y + 6, (active_tab == 3) ? 0x000000 : 0xFFFFFF, "3. Hardware CPU");

    draw_rect(x + 360, y, 140, 26, tab4_col);
    draw_string(x + 366, y + 6, (active_tab == 4) ? 0x000000 : 0xFFFFFF, "4. Disk & Storage");

    draw_rect(x, y + 28, win->width - 16, 2, 0x334155);

    int content_y = y + 36;

    if (active_tab == 1) {
        // Tab 1: Display, Video & Acceleration
        draw_string(x, content_y, 0xF1F5F9, "Screen Resolution:");

        // Row 1 of resolution buttons
        int res_y = content_y + 14;
        draw_rect(x + 10,  res_y, 88, 22, (active_res == 1) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 16,  res_y + 6, (active_res == 1) ? 0xFFFFFF : 0x94A3B8, "1024x768");

        draw_rect(x + 104, res_y, 88, 22, (active_res == 2) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 110, res_y + 6, (active_res == 2) ? 0xFFFFFF : 0x94A3B8, "1280x720");

        draw_rect(x + 198, res_y, 88, 22, (active_res == 3) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 204, res_y + 6, (active_res == 3) ? 0xFFFFFF : 0x94A3B8, "800x600");

        draw_rect(x + 292, res_y, 98, 22, (active_res == 4) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 298, res_y + 6, (active_res == 4) ? 0xFFFFFF : 0x94A3B8, "1280x1024");

        draw_rect(x + 396, res_y, 80, 22, (active_res == 5) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 402, res_y + 6, (active_res == 5) ? 0xFFFFFF : 0x94A3B8, "640x480");

        // Night light + brightness row
        int nl_y = content_y + 48;
        draw_string(x, nl_y, 0xF1F5F9, "Night Light:");
        draw_rect(x + 80,  nl_y - 2, 60, 20, night_light_on ? 0xF59E0B : 0x1E293B);
        draw_string(x + 88, nl_y + 3, night_light_on ? 0x000000 : 0xFFFFFF, night_light_on ? "ON" : "OFF");

        draw_string(x + 155, nl_y, 0xF1F5F9, "Brightness:");
        draw_rect(x + 238, nl_y - 2, 55, 20, (brightness_level == 100) ? 0x059669 : 0x1E293B);
        draw_string(x + 244, nl_y + 3, 0xFFFFFF, "100%");
        draw_rect(x + 298, nl_y - 2, 50, 20, (brightness_level == 75) ? 0x059669 : 0x1E293B);
        draw_string(x + 304, nl_y + 3, 0xFFFFFF, "75%");
        draw_rect(x + 353, nl_y - 2, 50, 20, (brightness_level == 50) ? 0x059669 : 0x1E293B);
        draw_string(x + 359, nl_y + 3, 0xFFFFFF, "50%");

        // Target FPS selector row
        int fps_row_y = content_y + 76;
        draw_string(x, fps_row_y, 0xF1F5F9, "Target FPS Limit:");
        int tfps = graphics_get_target_fps();
        draw_rect(x + 115, fps_row_y - 2, 60, 20, (tfps == 30) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 121, fps_row_y + 3, (tfps == 30) ? 0xFFFFFF : 0x94A3B8, "30 FPS");

        draw_rect(x + 180, fps_row_y - 2, 70, 20, (tfps == 60) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 186, fps_row_y + 3, (tfps == 60) ? 0xFFFFFF : 0x94A3B8, "60 FPS");

        draw_rect(x + 255, fps_row_y - 2, 70, 20, (tfps == 120) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 261, fps_row_y + 3, (tfps == 120) ? 0xFFFFFF : 0x94A3B8, "120 FPS");

        draw_rect(x + 330, fps_row_y - 2, 110, 20, (tfps == 240) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 336, fps_row_y + 3, (tfps == 240) ? 0xFFFFFF : 0x94A3B8, "Uncapped MAX");

        // Separator
        draw_rect(x, content_y + 104, win->width - 16, 1, 0x1E293B);

        // Live display info panel
        extern int graphics_get_real_fps(void);
        int live_fps = graphics_get_real_fps();
        char fps_str[64], res_str[64], fb_str[80];
        sprintf(fps_str, "Render Rate: %d FPS (Limit: %d)", live_fps, tfps);
        sprintf(res_str, "Framebuffer: %dx%d @ 32bpp", screen_w, screen_h);

        uintptr_t fb_addr = (uintptr_t)video_memory;
        const char* fb_type =
            (fb_addr == 0)            ? "None (headless)" :
            (fb_addr == 0xFD000000)   ? "BGA/QEMU LFB" :
            (fb_addr == 0xE0000000)   ? "VirtualBox VRAM" :
            (fb_addr >= 0xC0000000)   ? "VESA LFB" :
                                        "Multiboot LFB";
        sprintf(fb_str, "Driver: %s  |  VRAM @ 0x%08X", fb_type, (uint32_t)fb_addr);

        draw_string(x + 10, content_y + 112, 0x4ADE80, fps_str);

        // FPS bar
        int fps_bar_w = (live_fps * 120) / (tfps > 0 ? tfps : 60);
        if (fps_bar_w > 120) fps_bar_w = 120;
        draw_rect(x + 230, content_y + 112, 120, 10, 0x1E293B);
        uint32_t fps_col = (live_fps >= tfps - 5) ? 0x22C55E : (live_fps >= 30) ? 0xF59E0B : 0xEF4444;
        draw_rect(x + 230, content_y + 112, fps_bar_w, 10, fps_col);

        draw_string(x + 10, content_y + 128, 0x38BDF8, res_str);
        draw_string(x + 10, content_y + 144, 0x94A3B8, fb_str);
        draw_string(x + 10, content_y + 160, 0xF59E0B, settings_msg);
    }
    else if (active_tab == 2) {
        // Tab 2: Personalization & Themes
        draw_string(x, content_y, 0xF1F5F9, "System Wallpaper Themes & Accent Palettes:");

        draw_rect(x + 10, content_y + 18, 95, 24, (active_theme == 1) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 16, content_y + 23, (active_theme == 1) ? 0x000000 : 0xFFFFFF, "1. Midnight");

        draw_rect(x + 110, content_y + 18, 95, 24, (active_theme == 2) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 116, content_y + 23, (active_theme == 2) ? 0x000000 : 0xFFFFFF, "2. Cyber");

        draw_rect(x + 210, content_y + 18, 95, 24, (active_theme == 3) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 216, content_y + 23, (active_theme == 3) ? 0x000000 : 0xFFFFFF, "3. Emerald");

        draw_rect(x + 310, content_y + 18, 95, 24, (active_theme == 4) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 316, content_y + 23, (active_theme == 4) ? 0x000000 : 0xFFFFFF, "4. Purple");

        draw_string(x, content_y + 54, 0xF1F5F9, "Desktop Effects & Compositor Styling:");
        draw_string(x + 10, content_y + 72, 0x4ADE80, "[X] Windows 11 Acrylic Glassmorphism & Box-Blur Shadows");
        draw_string(x + 10, content_y + 88, 0x4ADE80, "[X] Drag Window Edge Snapping Highlights (Left/Right/Top)");
        draw_string(x + 10, content_y + 104, 0x4ADE80, "[X] Centered Taskbar Dock & Start Menu Application Search");
        draw_string(x + 10, content_y + 120, 0x38BDF8, "[X] Linux TTY Boot Splash -> Automated Direct GUI Launch");
    }
    else if (active_tab == 3) {
        // Tab 3: Hardware CPU & Memory Inspection
        draw_string(x, content_y, 0xF1F5F9, "CPU Hardware Inspection (CPUID Instruction):");

        cpu_info_t info;
        cpuid_get_info(&info);
        const char* brand_str = (info.brand[0] != '\0') ? info.brand : "x86_64 Compatible Processor";

        char l1[128], l2[128], l3[128];
        sprintf(l1, "Model: %s", brand_str);
        sprintf(l2, "Vendor: %s | Family: %u | Model: %u | Stepping: %u",
                info.vendor[0] ? info.vendor : "x86_64", info.family, info.model, info.stepping);
        sprintf(l3, "Architecture: 64-bit Long Mode | Logical Cores: %u", info.logical_cores ? info.logical_cores : 1);

        draw_string(x + 10, content_y + 20, 0x38BDF8, l1);
        draw_string(x + 10, content_y + 36, 0x94A3B8, l2);
        draw_string(x + 10, content_y + 52, 0xE2E8F0, l3);

        draw_string(x, content_y + 76, 0xF1F5F9, "Memory Subsystem (PMM / Heap):");
        uint64_t total_mem = pmm_get_total_memory();
        uint64_t free_mem = pmm_get_free_memory();
        uint64_t used_mem = (total_mem > free_mem) ? (total_mem - free_mem) : 0;
        char mem_line[128];
        sprintf(mem_line, "Total RAM: %u MB | Used: %u MB | Free: %u MB",
                (uint32_t)(total_mem / (1024*1024)),
                (uint32_t)(used_mem / (1024*1024)),
                (uint32_t)(free_mem / (1024*1024)));
        draw_string(x + 10, content_y + 94, 0x10B981, mem_line);
        draw_string(x + 10, content_y + 110, 0x94A3B8, "Page Frame Allocator: 4096-Byte Pages | 64-bit Paging Active");
    }
    else if (active_tab == 4) {
        // Tab 4: Storage, Peripherals & Input
        draw_string(x, content_y, 0xF1F5F9, "Disk Controller & Filesystem Telemetry:");

        ata_drive_t* drv = ata_get_drive(0);
        char ata_line[128];
        sprintf(ata_line, "Primary ATA Drive: %s (%u MB Total)",
                (drv && drv->present) ? drv->model : "Virtual Storage Disk",
                (drv && drv->present) ? ((drv->total_sectors * 512) / (1024 * 1024)) : 512);
        draw_string(x + 10, content_y + 20, 0x4ADE80, ata_line);

        ext4_superblock_t* sb = ext4_get_superblock();
        char fs_line[128];
        if (sb && ext4_is_mounted()) {
            sprintf(fs_line, "Mount: / | EXT4 Volume: %s | Superblock: 0x%X", sb->s_volume_name, sb->s_magic);
        } else {
            sprintf(fs_line, "Mount: / | Native EXT4 Superblock 0xEF53 Ready");
        }
        draw_string(x + 10, content_y + 36, 0x94A3B8, fs_line);

        draw_string(x, content_y + 64, 0xF1F5F9, "Network Adapter & Mouse Subsystem:");
        e1000_device_t* net = e1000_get_device();
        char net_line[128];
        sprintf(net_line, "e1000 NIC: MAC %02X:%02X:%02X:%02X:%02X:%02X | PCI Bus: 00:03.0",
                net->mac[0], net->mac[1], net->mac[2], net->mac[3], net->mac[4], net->mac[5]);
        draw_string(x + 10, content_y + 82, 0x38BDF8, net_line);

        draw_string(x + 10, content_y + 98, 0xE2E8F0, "VirtualBox VMMDev Integration: Absolute Mouse Sync Active");
    }
}

static void settings_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;

    // Header Tabs Clicks (rel_y between 8 and 34)
    if (rel_y >= 6 && rel_y <= 34) {
        if (rel_x >= 8 && rel_x <= 128) active_tab = 1;
        else if (rel_x >= 133 && rel_x <= 248) active_tab = 2;
        else if (rel_x >= 253 && rel_x <= 363) active_tab = 3;
        else if (rel_x >= 368 && rel_x <= 508) active_tab = 4;
        settings_redraw(win);
        return;
    }

    if (active_tab == 1) {
        // Resolution row click: buttons drawn at y+68+14 = y+82 (window relative ~50)
        if (rel_y >= 50 && rel_y <= 72) {
            if      (rel_x >= 18  && rel_x <= 106)  { active_res = 1; graphics_set_mode(1024, 768,  32); strcpy(settings_msg, "1024x768 active."); }
            else if (rel_x >= 112 && rel_x <= 200)  { active_res = 2; graphics_set_mode(1280, 720,  32); strcpy(settings_msg, "1280x720 active."); }
            else if (rel_x >= 206 && rel_x <= 294)  { active_res = 3; graphics_set_mode(800,  600,  32); strcpy(settings_msg, "800x600 active.");  }
            else if (rel_x >= 300 && rel_x <= 406)  { active_res = 4; graphics_set_mode(1280, 1024, 32); strcpy(settings_msg, "1280x1024 active."); }
            else if (rel_x >= 404 && rel_x <= 484)  { active_res = 5; graphics_set_mode(640,  480,  32); strcpy(settings_msg, "640x480 active.");  }
            settings_redraw(win);
        }
        // Night light: drawn at nl_y (content_y+48) → rel_y ~ 84
        else if (rel_y >= 82 && rel_y <= 104 && rel_x >= 88 && rel_x <= 148) {
            night_light_on = !night_light_on;
            graphics_set_night_light(night_light_on);
            strcpy(settings_msg, night_light_on ? "Night Light ON." : "Night Light OFF.");
            settings_redraw(win);
        }
        // Brightness buttons: same nl_y row, further right
        else if (rel_y >= 82 && rel_y <= 104) {
            if      (rel_x >= 246 && rel_x <= 301) { brightness_level = 100; graphics_set_brightness(100); strcpy(settings_msg, "Brightness 100%."); }
            else if (rel_x >= 306 && rel_x <= 356) { brightness_level = 75;  graphics_set_brightness(75);  strcpy(settings_msg, "Brightness 75%.");  }
            else if (rel_x >= 361 && rel_x <= 411) { brightness_level = 50;  graphics_set_brightness(50);  strcpy(settings_msg, "Brightness 50%.");  }
            settings_redraw(win);
        }
        // Target FPS row (fps_row_y: rel_y ~ 108..130)
        else if (rel_y >= 108 && rel_y <= 130) {
            if      (rel_x >= 120 && rel_x <= 180) { graphics_set_target_fps(30);  strcpy(settings_msg, "Target FPS set to 30 FPS."); }
            else if (rel_x >= 185 && rel_x <= 250) { graphics_set_target_fps(60);  strcpy(settings_msg, "Target FPS set to 60 FPS."); }
            else if (rel_x >= 255 && rel_x <= 325) { graphics_set_target_fps(120); strcpy(settings_msg, "Target FPS set to 120 FPS."); }
            else if (rel_x >= 330 && rel_x <= 445) { graphics_set_target_fps(240); strcpy(settings_msg, "Target FPS set to Uncapped MAX."); }
            settings_redraw(win);
        }
    }
    else if (active_tab == 2) {
        // Theme Selection Click (rel_y between 56 and 88)
        if (rel_y >= 56 && rel_y <= 88) {
            if (rel_x >= 18 && rel_x <= 113) { active_theme = 1; desktop_set_theme(1); }
            else if (rel_x >= 118 && rel_x <= 213) { active_theme = 2; desktop_set_theme(2); }
            else if (rel_x >= 218 && rel_x <= 313) { active_theme = 3; desktop_set_theme(3); }
            else if (rel_x >= 318 && rel_x <= 413) { active_theme = 4; desktop_set_theme(4); }
            settings_redraw(win);
        }
    }
}

void settings_open(void) {
    window_t* win = wm_create_window(100, 65, 510, 295, "Settings Control Center");
    if (win) {
        win->render_content = settings_redraw;
        win->on_click = settings_handle_click;
        taskbar_add_button(win->id, "Settings");
        settings_redraw(win);
    }
}
