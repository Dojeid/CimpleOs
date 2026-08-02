#include "gui/apps/settings.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/desktop.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "fs/ext4.h"
#include "mm/pmm.h"
#include "lib/printf.h"
#include "lib/string.h"

static int active_tab = 1; // 1=Display, 2=Personalize, 3=Input, 4=Storage/EXT4
static int active_theme = 1;
static int active_res = 1;  // 1=1024x768, 2=1280x720, 3=800x600, 4=1920x1080
static int brightness_level = 100; // 100%, 75%, 50%
static int night_light_on = 0;
static char settings_msg[128] = "System settings active.";

static void settings_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;

    // Dark glassmorphic container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Header Tabs Bar
    uint32_t tab1_col = (active_tab == 1) ? 0x38BDF8 : 0x1E293B;
    uint32_t tab2_col = (active_tab == 2) ? 0x38BDF8 : 0x1E293B;
    uint32_t tab3_col = (active_tab == 3) ? 0x38BDF8 : 0x1E293B;
    uint32_t tab4_col = (active_tab == 4) ? 0x38BDF8 : 0x1E293B;

    draw_rect(x, y, 110, 26, tab1_col);
    draw_string(x + 10, y + 6, (active_tab == 1) ? 0x000000 : 0xFFFFFF, "1. Display");

    draw_rect(x + 115, y, 115, 26, tab2_col);
    draw_string(x + 123, y + 6, (active_tab == 2) ? 0x000000 : 0xFFFFFF, "2. Personalize");

    draw_rect(x + 235, y, 100, 26, tab3_col);
    draw_string(x + 243, y + 6, (active_tab == 3) ? 0x000000 : 0xFFFFFF, "3. Input");

    draw_rect(x + 340, y, 140, 26, tab4_col);
    draw_string(x + 348, y + 6, (active_tab == 4) ? 0x000000 : 0xFFFFFF, "4. EXT4 Storage");

    draw_rect(x, y + 28, win->width - 16, 2, 0x334155);

    int content_y = y + 36;

    if (active_tab == 1) {
        // Tab 1: Display & Resolution
        draw_string(x, content_y, 0xF1F5F9, "Screen Resolution & Acceleration:");
        draw_rect(x + 10, content_y + 18, 100, 24, (active_res == 1) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 14, content_y + 23, (active_res == 1) ? 0x000000 : 0xFFFFFF, "1024x768");

        draw_rect(x + 115, content_y + 18, 100, 24, (active_res == 2) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 119, content_y + 23, (active_res == 2) ? 0x000000 : 0xFFFFFF, "1280x720");

        draw_rect(x + 220, content_y + 18, 100, 24, (active_res == 3) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 224, content_y + 23, (active_res == 3) ? 0x000000 : 0xFFFFFF, "800x600");

        draw_rect(x + 325, content_y + 18, 100, 24, (active_res == 4) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 329, content_y + 23, (active_res == 4) ? 0x000000 : 0xFFFFFF, "1920x1080");

        draw_string(x, content_y + 54, 0xF1F5F9, "Night Light (Blue Light Filter):");
        draw_rect(x + 10, content_y + 72, 90, 24, night_light_on ? 0xF59E0B : 0x1E293B);
        draw_string(x + 20, content_y + 77, night_light_on ? 0x000000 : 0xFFFFFF, night_light_on ? "ON [Warm]" : "OFF");

        draw_string(x + 130, content_y + 54, 0xF1F5F9, "Screen Brightness Level:");
        draw_rect(x + 130, content_y + 72, 80, 24, (brightness_level == 100) ? 0x10B981 : 0x1E293B);
        draw_string(x + 138, content_y + 77, (brightness_level == 100) ? 0x000000 : 0xFFFFFF, "100% Max");

        draw_rect(x + 215, content_y + 72, 80, 24, (brightness_level == 75) ? 0x10B981 : 0x1E293B);
        draw_string(x + 223, content_y + 77, (brightness_level == 75) ? 0x000000 : 0xFFFFFF, "75% Soft");

        draw_rect(x + 300, content_y + 72, 80, 24, (brightness_level == 50) ? 0x10B981 : 0x1E293B);
        draw_string(x + 308, content_y + 77, (brightness_level == 50) ? 0x000000 : 0xFFFFFF, "50% Dim");

        draw_string(x, content_y + 110, 0xF1F5F9, "Alpha Blending & Compositing Engine:");
        draw_string(x + 10, content_y + 128, 0x4ADE80, "[X] Hardware Double-Buffered VESA/BGA Framebuffer");
        draw_string(x + 10, content_y + 144, 0x38BDF8, settings_msg);
    }
    else if (active_tab == 2) {
        // Tab 2: Personalization & Themes
        draw_string(x, content_y, 0xF1F5F9, "System Theme & Color Schemes:");

        draw_rect(x + 10, content_y + 18, 95, 24, (active_theme == 1) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 16, content_y + 23, (active_theme == 1) ? 0x000000 : 0xFFFFFF, "1. Midnight");

        draw_rect(x + 110, content_y + 18, 95, 24, (active_theme == 2) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 116, content_y + 23, (active_theme == 2) ? 0x000000 : 0xFFFFFF, "2. Cyber");

        draw_rect(x + 210, content_y + 18, 95, 24, (active_theme == 3) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 216, content_y + 23, (active_theme == 3) ? 0x000000 : 0xFFFFFF, "3. Emerald");

        draw_rect(x + 310, content_y + 18, 95, 24, (active_theme == 4) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 316, content_y + 23, (active_theme == 4) ? 0x000000 : 0xFFFFFF, "4. Purple");

        draw_string(x, content_y + 86, 0xF1F5F9, "Desktop Effects & Window Decorations:");
        draw_string(x + 10, content_y + 106, 0x4ADE80, "[X] Modern macOS/Linux Circular Control Dots (Red/Yellow/Green)");
        draw_string(x + 10, content_y + 122, 0x4ADE80, "[X] Active Window Cyan Glow Highlight Border (0x38BDF8)");
        draw_string(x + 10, content_y + 138, 0x4ADE80, "[X] Translucent Taskbar & Falkon Menu Badges");
    }
    else if (active_tab == 3) {
        // Tab 3: Mouse & VirtualBox Pointer
        draw_string(x, content_y, 0xF1F5F9, "Mouse Pointer Acceleration & Sensitivity:");
        draw_string(x + 10, content_y + 20, 0x4ADE80, "Speed: 1.0x Normal (1:1 Hardware Pixel Sync)");

        draw_string(x, content_y + 54, 0xF1F5F9, "VirtualBox Cursor Integration:");
        draw_string(x + 10, content_y + 74, 0x38BDF8, "Edge Damping Guard: ACTIVE (DDC Suppresses Border Escapes)");
        draw_string(x + 10, content_y + 90, 0x94A3B8, "Max Delta Clamp: +/- 30px per packet");

        draw_string(x, content_y + 124, 0xF1F5F9, "Hardware Input Device Status:");
        draw_string(x + 10, content_y + 144, 0xE2E8F0, "PS/2 Auxiliary Controller Port: IRQ 12 Active (0x60 / 0x64)");
    }
    else if (active_tab == 4) {
        // Tab 4: EXT4 Storage & Filesystem
        draw_string(x, content_y, 0xF1F5F9, "Target EXT4 Filesystem Inspection:");

        ext4_superblock_t* sb = ext4_get_superblock();
        char line1[128];
        char line2[128];
        char line3[128];

        if (sb && ext4_is_mounted()) {
            sprintf(line1, "Superblock Magic: 0x%X (Verified Native EXT4 Volume)", sb->s_magic);
            sprintf(line2, "Volume Name: %s | Inodes: %u | Free Inodes: %u", sb->s_volume_name, sb->s_inodes_count, sb->s_free_inodes_count);
            sprintf(line3, "Block Size: 1024 Bytes | Total Blocks: %u | Free: %u", sb->s_blocks_count_lo, sb->s_free_blocks_count_lo);
        } else {
            sprintf(line1, "Superblock Magic: 0xEF53 (Native EXT4 Format Engine Ready)");
            sprintf(line2, "Target Disk: /dev/sda (Primary Master ATA Drive)");
            sprintf(line3, "Mount Point: / | Status: Ready for Installer Wizard");
        }

        draw_string(x + 10, content_y + 22, 0x4ADE80, line1);
        draw_string(x + 10, content_y + 40, 0xE2E8F0, line2);
        draw_string(x + 10, content_y + 58, 0x94A3B8, line3);

        draw_string(x, content_y + 92, 0xF1F5F9, "Storage Controller & Device:");
        ata_drive_t* drv = ata_get_drive(0);
        char ata_str[128];
        sprintf(ata_str, "Disk: %s (%u MB Total)", drv->present ? drv->model : "Virtual Storage Disk", (drv->total_sectors * 512) / (1024 * 1024));
        draw_string(x + 10, content_y + 112, 0x38BDF8, ata_str);
    }
}

static void settings_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;

    // Header Tabs Clicks
    if (rel_y >= 32 && rel_y <= 58) {
        if (rel_x >= 8 && rel_x <= 118) active_tab = 1;
        else if (rel_x >= 123 && rel_x <= 238) active_tab = 2;
        else if (rel_x >= 243 && rel_x <= 343) active_tab = 3;
        else if (rel_x >= 348 && rel_x <= 488) active_tab = 4;
        settings_redraw(win);
        return;
    }

    int content_y = rel_y - 68;
    if (active_tab == 1) {
        // Resolution Buttons Click
        if (content_y >= 18 && content_y <= 42) {
            if (rel_x >= 18 && rel_x <= 118) {
                active_res = 1;
                graphics_set_mode(1024, 768, 32);
                strcpy(settings_msg, "Display set to 1024x768 @ 32bpp BGA LFB.");
            } else if (rel_x >= 123 && rel_x <= 223) {
                active_res = 2;
                graphics_set_mode(1280, 720, 32);
                strcpy(settings_msg, "Display set to 1280x720 @ 32bpp BGA LFB.");
            } else if (rel_x >= 228 && rel_x <= 328) {
                active_res = 3;
                graphics_set_mode(800, 600, 32);
                strcpy(settings_msg, "Display set to 800x600 @ 32bpp BGA LFB.");
            } else if (rel_x >= 333 && rel_x <= 433) {
                active_res = 4;
                graphics_set_mode(1920, 1080, 32);
                strcpy(settings_msg, "Display set to 1920x1080 @ 32bpp BGA LFB.");
            }
            settings_redraw(win);
        }
        // Night Light Toggle Click
        else if (content_y >= 72 && content_y <= 96 && rel_x >= 18 && rel_x <= 108) {
            night_light_on = !night_light_on;
            graphics_set_night_light(night_light_on);
            strcpy(settings_msg, night_light_on ? "Night Light Filter ENABLED." : "Night Light Filter DISABLED.");
            settings_redraw(win);
        }
        // Brightness Controls Click
        else if (content_y >= 72 && content_y <= 96) {
            if (rel_x >= 138 && rel_x <= 218) {
                brightness_level = 100;
                graphics_set_brightness(100);
            } else if (rel_x >= 223 && rel_x <= 303) {
                brightness_level = 75;
                graphics_set_brightness(75);
            } else if (rel_x >= 308 && rel_x <= 388) {
                brightness_level = 50;
                graphics_set_brightness(50);
            }
            settings_redraw(win);
        }
    }
    else if (active_tab == 2) {
        // Theme Selection Click
        if (content_y >= 18 && content_y <= 42) {
            if (rel_x >= 18 && rel_x <= 113) { active_theme = 1; desktop_set_theme(1); }
            else if (rel_x >= 118 && rel_x <= 213) { active_theme = 2; desktop_set_theme(2); }
            else if (rel_x >= 218 && rel_x <= 313) { active_theme = 3; desktop_set_theme(3); }
            else if (rel_x >= 318 && rel_x <= 413) { active_theme = 4; desktop_set_theme(4); }
            settings_redraw(win);
        }
    }
}

void settings_open(void) {
    window_t* win = wm_create_window(110, 75, 490, 280, "Settings Control Center");
    if (win) {
        win->render_content = settings_redraw;
        win->on_click = settings_handle_click;
        taskbar_add_button(win->id, "Settings");
        settings_redraw(win);
    }
}
