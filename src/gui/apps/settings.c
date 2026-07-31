#include "gui/apps/settings.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/desktop.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "fs/ext4.h"
#include "mm/pmm.h"
#include "lib/printf.h"

static int active_tab = 1; // 1=Display, 2=Personalize, 3=Input, 4=Storage/EXT4
static int active_theme = 1;
static int active_res = 1;  // 1=1024x768, 2=1280x720, 3=800x600
static int active_fps = 2;  // 1=30, 2=60, 3=Uncapped
static int brightness_level = 100; // 100%, 75%, 50%

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
        draw_rect(x + 10, content_y + 18, 120, 24, (active_res == 1) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 16, content_y + 23, (active_res == 1) ? 0x000000 : 0xFFFFFF, "1024 x 768");

        draw_rect(x + 135, content_y + 18, 120, 24, (active_res == 2) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 141, content_y + 23, (active_res == 2) ? 0x000000 : 0xFFFFFF, "1280 x 720");

        draw_rect(x + 260, content_y + 18, 120, 24, (active_res == 3) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 266, content_y + 23, (active_res == 3) ? 0x000000 : 0xFFFFFF, "800 x 600");

        draw_string(x, content_y + 54, 0xF1F5F9, "Frame Rate Pacing Target:");
        draw_rect(x + 10, content_y + 72, 100, 24, (active_fps == 1) ? 0x10B981 : 0x1E293B);
        draw_string(x + 20, content_y + 77, (active_fps == 1) ? 0x000000 : 0xFFFFFF, "30 FPS");

        draw_rect(x + 115, content_y + 72, 100, 24, (active_fps == 2) ? 0x10B981 : 0x1E293B);
        draw_string(x + 125, content_y + 77, (active_fps == 2) ? 0x000000 : 0xFFFFFF, "60 FPS");

        draw_rect(x + 220, content_y + 72, 120, 24, (active_fps == 3) ? 0x10B981 : 0x1E293B);
        draw_string(x + 230, content_y + 77, (active_fps == 3) ? 0x000000 : 0xFFFFFF, "Max Uncapped");

        draw_string(x, content_y + 108, 0xF1F5F9, "Screen Brightness Level:");
        draw_rect(x + 10, content_y + 126, 90, 24, (brightness_level == 100) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 18, content_y + 131, (brightness_level == 100) ? 0x000000 : 0xFFFFFF, "100% Full");

        draw_rect(x + 105, content_y + 126, 90, 24, (brightness_level == 75) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 113, content_y + 131, (brightness_level == 75) ? 0x000000 : 0xFFFFFF, "75% Soft");

        draw_rect(x + 200, content_y + 126, 90, 24, (brightness_level == 50) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 208, content_y + 131, (brightness_level == 50) ? 0x000000 : 0xFFFFFF, "50% Dim");

        draw_string(x + 10, content_y + 160, 0x4ADE80, "VBE Driver: Bochs BGA / PCI LFB Active @ 0xFD000000");
    }
    else if (active_tab == 2) {
        // Tab 2: Personalization & Themes
        draw_string(x, content_y, 0xF1F5F9, "System Wallpaper & Color Schemes:");

        draw_rect(x + 10, content_y + 18, 95, 24, (active_theme == 1) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 16, content_y + 23, (active_theme == 1) ? 0x000000 : 0xFFFFFF, "1. Midnight");

        draw_rect(x + 110, content_y + 18, 95, 24, (active_theme == 2) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 116, content_y + 23, (active_theme == 2) ? 0x000000 : 0xFFFFFF, "2. Cyber");

        draw_rect(x + 210, content_y + 18, 95, 24, (active_theme == 3) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 216, content_y + 23, (active_theme == 3) ? 0x000000 : 0xFFFFFF, "3. Emerald");

        draw_rect(x + 310, content_y + 18, 95, 24, (active_theme == 4) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 316, content_y + 23, (active_theme == 4) ? 0x000000 : 0xFFFFFF, "4. Purple");

        draw_rect(x + 10, content_y + 48, 95, 24, (active_theme == 5) ? 0x38BDF8 : 0x1E293B);
        draw_string(x + 16, content_y + 53, (active_theme == 5) ? 0x000000 : 0xFFFFFF, "5. Synth");

        draw_string(x, content_y + 86, 0xF1F5F9, "Desktop Effects & Window Decorations:");
        draw_string(x + 10, content_y + 106, 0x4ADE80, "[X] Modern macOS/Linux Circular Dot Controls (Red/Yellow/Green)");
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

void settings_open(void) {
    window_t* win = wm_create_window(110, 75, 490, 280, "Settings Control Center");
    if (win) {
        win->render_content = settings_redraw;
        taskbar_add_button(win->id, "Settings");
        settings_redraw(win);
    }
}
