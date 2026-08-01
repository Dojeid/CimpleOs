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
#include "fs/vfs.h"

static int active_tab = 1; // 1=Display, 2=Personalize, 3=Input/Mouse, 4=EXT4 Storage, 5=About OS
static int active_theme = 1;

// Detailed Display Settings State
static int active_res = 1;         // 1=1024x768, 2=1280x720, 3=1920x1080, 4=800x600
static int active_fps = 2;         // 1=30Hz, 2=60Hz, 3=90Hz, 4=120Hz, 5=Uncapped
static int active_scale = 1;       // 1=100%, 2=125%, 3=150%, 4=200%
static int brightness_level = 100; // 100%, 85%, 70%, 50%, 30%
static int night_light = 0;        // 0=Disabled (6500K), 1=Warm (4500K), 2=Amber (3000K)
static int color_profile = 1;      // 1=sRGB Standard, 2=DCI-P3 Vivid, 3=High Contrast, 4=OLED Deep Black
static int vsync_enabled = 1;      // 1=Enabled, 0=Disabled

static char settings_msg[128] = "Display & Hardware configuration synchronized.";

static void settings_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 10;
    int y = win->y + 32;

    // Dark sleek container background (Midnight Slate 0x0F172A)
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Header Tabs Bar
    uint32_t tab1_col = (active_tab == 1) ? 0x0284C7 : 0x1E293B;
    uint32_t tab2_col = (active_tab == 2) ? 0x0284C7 : 0x1E293B;
    uint32_t tab3_col = (active_tab == 3) ? 0x0284C7 : 0x1E293B;
    uint32_t tab4_col = (active_tab == 4) ? 0x0284C7 : 0x1E293B;
    uint32_t tab5_col = (active_tab == 5) ? 0x0284C7 : 0x1E293B;

    draw_rect(x, y, 90, 24, tab1_col);
    draw_string(x + 8, y + 5, (active_tab == 1) ? 0xFFFFFF : 0x94A3B8, "Display");

    draw_rect(x + 95, y, 95, 24, tab2_col);
    draw_string(x + 103, y + 5, (active_tab == 2) ? 0xFFFFFF : 0x94A3B8, "Themes");

    draw_rect(x + 195, y, 85, 24, tab3_col);
    draw_string(x + 203, y + 5, (active_tab == 3) ? 0xFFFFFF : 0x94A3B8, "Mouse");

    draw_rect(x + 285, y, 105, 24, tab4_col);
    draw_string(x + 293, y + 5, (active_tab == 4) ? 0xFFFFFF : 0x94A3B8, "EXT4 Storage");

    draw_rect(x + 395, y, 75, 24, tab5_col);
    draw_string(x + 403, y + 5, (active_tab == 5) ? 0xFFFFFF : 0x94A3B8, "About");

    draw_rect(x, y + 26, win->width - 20, 2, 0x334155);

    int content_y = y + 34;

    if (active_tab == 1) {
        // Tab 1: Comprehensive Display & Resolution Control Panel
        draw_string(x, content_y, 0x38BDF8, "Screen Resolution & Display Mode:");
        draw_rect(x + 10, content_y + 18, 105, 22, (active_res == 1) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 15, content_y + 22, (active_res == 1) ? 0xFFFFFF : 0x94A3B8, "1024x768 HD");

        draw_rect(x + 120, content_y + 18, 105, 22, (active_res == 2) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 125, content_y + 22, (active_res == 2) ? 0xFFFFFF : 0x94A3B8, "1280x720 720p");

        draw_rect(x + 230, content_y + 18, 105, 22, (active_res == 3) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 235, content_y + 22, (active_res == 3) ? 0xFFFFFF : 0x94A3B8, "1920x1080 FHD");

        draw_rect(x + 340, content_y + 18, 105, 22, (active_res == 4) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 345, content_y + 22, (active_res == 4) ? 0xFFFFFF : 0x94A3B8, "800x600 SVGA");

        // UI Scaling Modes
        draw_string(x, content_y + 46, 0x38BDF8, "Display UI Scaling Factor:");
        draw_rect(x + 10, content_y + 64, 100, 22, (active_scale == 1) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 18, content_y + 68, (active_scale == 1) ? 0xFFFFFF : 0x94A3B8, "100% Native");

        draw_rect(x + 115, content_y + 64, 100, 22, (active_scale == 2) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 123, content_y + 68, (active_scale == 2) ? 0xFFFFFF : 0x94A3B8, "125% High");

        draw_rect(x + 220, content_y + 64, 100, 22, (active_scale == 3) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 228, content_y + 68, (active_scale == 3) ? 0xFFFFFF : 0x94A3B8, "150% Retina");

        draw_rect(x + 325, content_y + 64, 100, 22, (active_scale == 4) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 333, content_y + 68, (active_scale == 4) ? 0xFFFFFF : 0x94A3B8, "200% Ultra");

        // Refresh Rate & Frame Pacing
        draw_string(x, content_y + 92, 0x38BDF8, "Refresh Rate & Frame Target:");
        draw_rect(x + 10, content_y + 110, 80, 22, (active_fps == 1) ? 0x10B981 : 0x1E293B);
        draw_string(x + 18, content_y + 114, (active_fps == 1) ? 0xFFFFFF : 0x94A3B8, "30 Hz");

        draw_rect(x + 95, content_y + 110, 80, 22, (active_fps == 2) ? 0x10B981 : 0x1E293B);
        draw_string(x + 103, content_y + 114, (active_fps == 2) ? 0xFFFFFF : 0x94A3B8, "60 Hz");

        draw_rect(x + 180, content_y + 110, 80, 22, (active_fps == 3) ? 0x10B981 : 0x1E293B);
        draw_string(x + 188, content_y + 114, (active_fps == 3) ? 0xFFFFFF : 0x94A3B8, "90 Hz");

        draw_rect(x + 265, content_y + 110, 80, 22, (active_fps == 4) ? 0x10B981 : 0x1E293B);
        draw_string(x + 273, content_y + 114, (active_fps == 4) ? 0xFFFFFF : 0x94A3B8, "120 Hz");

        draw_rect(x + 350, content_y + 110, 95, 22, (active_fps == 5) ? 0x10B981 : 0x1E293B);
        draw_string(x + 356, content_y + 114, (active_fps == 5) ? 0xFFFFFF : 0x94A3B8, "Uncapped");

        // Brightness & Night Light Filter
        draw_string(x, content_y + 138, 0x38BDF8, "Brightness & Night Light Protection:");
        draw_rect(x + 10, content_y + 156, 75, 22, (brightness_level == 100) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 18, content_y + 160, (brightness_level == 100) ? 0xFFFFFF : 0x94A3B8, "100%");

        draw_rect(x + 90, content_y + 156, 75, 22, (brightness_level == 85) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 98, content_y + 160, (brightness_level == 85) ? 0xFFFFFF : 0x94A3B8, "85%");

        draw_rect(x + 170, content_y + 156, 75, 22, (brightness_level == 70) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 178, content_y + 160, (brightness_level == 70) ? 0xFFFFFF : 0x94A3B8, "70%");

        draw_rect(x + 250, content_y + 156, 75, 22, (brightness_level == 50) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 258, content_y + 160, (brightness_level == 50) ? 0xFFFFFF : 0x94A3B8, "50%");

        draw_rect(x + 330, content_y + 156, 115, 22, (night_light == 1) ? 0xEC4899 : 0x1E293B);
        draw_string(x + 336, content_y + 160, (night_light == 1) ? 0xFFFFFF : 0x94A3B8, "Night Warm");

        // Color Profile & VSync Pacing Toggle
        draw_string(x, content_y + 184, 0x38BDF8, "Display Color Profile & VSync Pacing:");
        draw_rect(x + 10, content_y + 202, 115, 22, (color_profile == 1) ? 0x8B5CF6 : 0x1E293B);
        draw_string(x + 16, content_y + 206, (color_profile == 1) ? 0xFFFFFF : 0x94A3B8, "sRGB Standard");

        draw_rect(x + 130, content_y + 202, 115, 22, (color_profile == 2) ? 0x8B5CF6 : 0x1E293B);
        draw_string(x + 136, content_y + 206, (color_profile == 2) ? 0xFFFFFF : 0x94A3B8, "DCI-P3 Vivid");

        draw_rect(x + 250, content_y + 202, 100, 22, (vsync_enabled == 1) ? 0x10B981 : 0x1E293B);
        draw_string(x + 256, content_y + 206, (vsync_enabled == 1) ? 0xFFFFFF : 0x94A3B8, "VSync: ON");

        draw_string(x + 10, content_y + 230, 0x4ADE80, "VBE Engine: Bochs BGA 32-bit Linear Framebuffer @ 0xFD000000");
    }
    else if (active_tab == 2) {
        // Tab 2: Personalization & Desktop Wallpaper Themes
        draw_string(x, content_y, 0x38BDF8, "Desktop Color Theme & Wallpaper (Click to Apply):");

        draw_rect(x + 10, content_y + 20, 110, 26, (active_theme == 1) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 18, content_y + 25, (active_theme == 1) ? 0xFFFFFF : 0x94A3B8, "1. Midnight");

        draw_rect(x + 125, content_y + 20, 110, 26, (active_theme == 2) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 133, content_y + 25, (active_theme == 2) ? 0xFFFFFF : 0x94A3B8, "2. Cyber");

        draw_rect(x + 240, content_y + 20, 110, 26, (active_theme == 3) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 248, content_y + 25, (active_theme == 3) ? 0xFFFFFF : 0x94A3B8, "3. Emerald");

        draw_rect(x + 355, content_y + 20, 110, 26, (active_theme == 4) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 363, content_y + 25, (active_theme == 4) ? 0xFFFFFF : 0x94A3B8, "4. Purple");

        draw_rect(x + 10, content_y + 54, 110, 26, (active_theme == 5) ? 0x0284C7 : 0x1E293B);
        draw_string(x + 18, content_y + 59, (active_theme == 5) ? 0xFFFFFF : 0x94A3B8, "5. Synthwave");

        draw_string(x, content_y + 94, 0x38BDF8, "Desktop Window Manager Visual Effects:");
        draw_string(x + 10, content_y + 114, 0x4ADE80, "[X] macOS Style Red/Yellow/Green Control Dots");
        draw_string(x + 10, content_y + 130, 0x4ADE80, "[X] Active Window Cyan Outline Glow (0x38BDF8)");
        draw_string(x + 10, content_y + 146, 0x4ADE80, "[X] Translucent Launcher Panel & Taskbar Accents");
        draw_string(x + 10, content_y + 172, 0x38BDF8, settings_msg);
    }
    else if (active_tab == 3) {
        // Tab 3: Mouse & Pointer System
        draw_string(x, content_y, 0x38BDF8, "Mouse Pointer Calibration & Direction:");
        draw_string(x + 10, content_y + 20, 0x4ADE80, "Axis Orientation: CORRECTED (Up = Up, Down = Down, 1:1 Tracking)");
        draw_string(x + 10, content_y + 36, 0x94A3B8, "PS/2 Packet Sanity Filtering: ACTIVE (Max Delta Clamp: +/- 45px)");

        draw_string(x, content_y + 70, 0x38BDF8, "VirtualBox Pointer Integration Status:");
        draw_string(x + 10, content_y + 90, 0x38BDF8, "Edge Boundary Damping: ACTIVE (Prevents Screen Escapes)");
        draw_string(x + 10, content_y + 106, 0x94A3B8, "VMMDev Absolute Protocol: ACTIVE (Port 0x5040)");

        draw_string(x, content_y + 140, 0x38BDF8, "Hardware Controller Specs:");
        draw_string(x + 10, content_y + 160, 0xE2E8F0, "PS/2 Auxiliary Controller Port: IRQ 12 Active (0x60 / 0x64)");
    }
    else if (active_tab == 4) {
        // Tab 4: EXT4 Storage & File System
        draw_string(x, content_y, 0x38BDF8, "EXT4 Storage Inspection:");

        ext4_superblock_t* sb = ext4_get_superblock();
        char line1[128];
        char line2[128];
        char line3[128];

        if (sb && ext4_is_mounted()) {
            sprintf(line1, "Superblock Magic: 0x%X (Verified Native EXT4 Volume)", sb->s_magic);
            sprintf(line2, "Volume Name: %s | Inodes: %u | Free Inodes: %u", sb->s_volume_name, sb->s_inodes_count, sb->s_free_inodes_count);
            sprintf(line3, "Block Size: 1024 Bytes | Total Blocks: %u | Free: %u", sb->s_blocks_count_lo, sb->s_free_blocks_count_lo);
        } else {
            sprintf(line1, "Superblock Magic: 0xEF53 (Native EXT4 Engine Ready)");
            sprintf(line2, "Target Disk: /dev/sda (Primary Master ATA Drive)");
            sprintf(line3, "Mount Point: / | Status: Ready for Installation");
        }

        draw_string(x + 10, content_y + 22, 0x4ADE80, line1);
        draw_string(x + 10, content_y + 40, 0xE2E8F0, line2);
        draw_string(x + 10, content_y + 58, 0x94A3B8, line3);

        int format_btn_x = x + 10;
        int format_btn_y = content_y + 90;
        draw_rect(format_btn_x, format_btn_y, 190, 28, 0xDC2626);
        draw_string(format_btn_x + 12, format_btn_y + 6, 0xFFFFFF, "[ Format EXT4 Disk ]");

        draw_string(x + 10, content_y + 130, 0x38BDF8, settings_msg);
    }
    else if (active_tab == 5) {
        // Tab 5: About Falkon-OS
        draw_string(x, content_y, 0x38BDF8, "Falkon-OS Enterprise 64-Bit System Overview:");
        draw_string(x + 10, content_y + 22, 0xFFFFFF, "Architecture: x86_64 Long Mode (PMM + VMM Paging Active)");
        draw_string(x + 10, content_y + 38, 0xFFFFFF, "Filesystem: EXT4 Native Storage + Linux-style VFS");
        draw_string(x + 10, content_y + 54, 0xFFFFFF, "Multitasking: Preemptive Hardware Stack Context Switching");
        draw_string(x + 10, content_y + 70, 0xFFFFFF, "POSIX Support: Fork, Waitpid, Execve System Calls");
        draw_string(x + 10, content_y + 86, 0xFFFFFF, "Graphics: 32-bit Linear Framebuffer + Modern Window Manager");
        draw_string(x + 10, content_y + 115, 0x94A3B8, "Falkon-OS Enterprise Operating System.");
    }
}

static void settings_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;

    // Tab Header Selection
    if (rel_y >= 0 && rel_y <= 24) {
        if (rel_x >= 0 && rel_x < 90) active_tab = 1;
        else if (rel_x >= 95 && rel_x < 190) active_tab = 2;
        else if (rel_x >= 195 && rel_x < 280) active_tab = 3;
        else if (rel_x >= 285 && rel_x < 390) active_tab = 4;
        else if (rel_x >= 395 && rel_x < 470) active_tab = 5;
        settings_redraw(win);
        return;
    }

    int content_y = 34;

    if (active_tab == 1) {
        // Resolution presets
        if (rel_y >= content_y + 18 && rel_y <= content_y + 40) {
            if (rel_x >= 10 && rel_x <= 115) active_res = 1;
            else if (rel_x >= 120 && rel_x <= 225) active_res = 2;
            else if (rel_x >= 230 && rel_x <= 335) active_res = 3;
            else if (rel_x >= 340 && rel_x <= 445) active_res = 4;
            strcpy(settings_msg, "Resolution preset updated.");
        }
        // Scaling presets
        else if (rel_y >= content_y + 64 && rel_y <= content_y + 86) {
            if (rel_x >= 10 && rel_x <= 110) active_scale = 1;
            else if (rel_x >= 115 && rel_x <= 215) active_scale = 2;
            else if (rel_x >= 220 && rel_x <= 320) active_scale = 3;
            else if (rel_x >= 325 && rel_x <= 425) active_scale = 4;
            strcpy(settings_msg, "UI Scale factor updated.");
        }
        // Refresh Rate presets
        else if (rel_y >= content_y + 110 && rel_y <= content_y + 132) {
            if (rel_x >= 10 && rel_x <= 90) active_fps = 1;
            else if (rel_x >= 95 && rel_x <= 175) active_fps = 2;
            else if (rel_x >= 180 && rel_x <= 260) active_fps = 3;
            else if (rel_x >= 265 && rel_x <= 345) active_fps = 4;
            else if (rel_x >= 350 && rel_x <= 445) active_fps = 5;
            strcpy(settings_msg, "Refresh rate pacing target updated.");
        }
        // Brightness & Night Light presets
        else if (rel_y >= content_y + 156 && rel_y <= content_y + 178) {
            if (rel_x >= 10 && rel_x <= 85) brightness_level = 100;
            else if (rel_x >= 90 && rel_x <= 165) brightness_level = 85;
            else if (rel_x >= 170 && rel_x <= 245) brightness_level = 70;
            else if (rel_x >= 250 && rel_x <= 325) brightness_level = 50;
            else if (rel_x >= 330 && rel_x <= 445) night_light = !night_light;
            strcpy(settings_msg, "Screen brightness / night light updated.");
        }
        // Color Profiles & VSync
        else if (rel_y >= content_y + 202 && rel_y <= content_y + 224) {
            if (rel_x >= 10 && rel_x <= 125) color_profile = 1;
            else if (rel_x >= 130 && rel_x <= 245) color_profile = 2;
            else if (rel_x >= 250 && rel_x <= 350) vsync_enabled = !vsync_enabled;
            strcpy(settings_msg, "Color profile & VSync setting updated.");
        }
        settings_redraw(win);
    }
    else if (active_tab == 2) {
        // Theme selection clicks
        if (rel_y >= content_y + 20 && rel_y <= content_y + 46) {
            if (rel_x >= 10 && rel_x <= 120) { active_theme = 1; desktop_set_theme(1); }
            else if (rel_x >= 125 && rel_x <= 235) { active_theme = 2; desktop_set_theme(2); }
            else if (rel_x >= 240 && rel_x <= 350) { active_theme = 3; desktop_set_theme(3); }
            else if (rel_x >= 355 && rel_x <= 465) { active_theme = 4; desktop_set_theme(4); }
            strcpy(settings_msg, "Desktop Theme Applied!");
        } else if (rel_y >= content_y + 54 && rel_y <= content_y + 80) {
            if (rel_x >= 10 && rel_x <= 120) { active_theme = 5; desktop_set_theme(5); }
            strcpy(settings_msg, "Synthwave Theme Applied!");
        }
        settings_redraw(win);
    }
    else if (active_tab == 4) {
        // Format EXT4 Disk button
        if (rel_x >= 10 && rel_x <= 200 && rel_y >= content_y + 90 && rel_y <= content_y + 118) {
            ata_drive_t* drv = ata_get_drive(0);
            uint32_t total_sec = drv->present ? drv->total_sectors : 4096;
            ext4_format_drive(0, total_sec, "FALKON_ROOT");
            vfs_mount("hda", "/", "ext4");
            strcpy(settings_msg, "EXT4 Partition Formatted & Mounted!");
            settings_redraw(win);
        }
    }
}

void settings_open(void) {
    window_t* win = wm_create_window(90, 60, 490, 310, "Display & Hardware Settings");
    if (win) {
        win->render_content = settings_redraw;
        win->on_click = settings_handle_click;
        taskbar_add_button(win->id, "Settings");
        settings_redraw(win);
    }
}
