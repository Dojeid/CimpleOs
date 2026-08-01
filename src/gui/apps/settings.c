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

static int active_tab = 1; // 1=Display, 2=Personalize, 3=Mouse, 4=EXT4 Storage, 5=About OS
static int active_theme = 1;

static int active_res = 1;         // 1=1024x768, 2=1280x720, 3=1920x1080, 4=800x600
static int active_fps = 2;         // 1=30Hz, 2=60Hz, 3=90Hz, 4=120Hz, 5=Uncapped
static int active_scale = 1;       // 1=100%, 2=125%, 3=150%, 4=200%
static int brightness_level = 100; // 100%, 85%, 70%, 50%
static int night_light = 0;        // 0=Disabled, 1=Warm Night Light
static int color_profile = 1;      // 1=sRGB, 2=DCI-P3
static int vsync_enabled = 1;

static char settings_msg[128] = "System Display & Hardware parameters synchronized.";

static void settings_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 10;
    int y = win->y + 32;

    gui_theme_t* theme = theme_get_current();

    // Dark glassmorphic container background
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Header Tabs Bar
    uint32_t tab1_col = (active_tab == 1) ? theme->accent_color : 0x1E293B;
    uint32_t tab2_col = (active_tab == 2) ? theme->accent_color : 0x1E293B;
    uint32_t tab3_col = (active_tab == 3) ? theme->accent_color : 0x1E293B;
    uint32_t tab4_col = (active_tab == 4) ? theme->accent_color : 0x1E293B;
    uint32_t tab5_col = (active_tab == 5) ? theme->accent_color : 0x1E293B;

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

    draw_rect(x, y + 26, win->width - 20, 2, theme->accent_color);

    int content_y = y + 34;

    if (active_tab == 1) {
        // Tab 1: Interactive Display Settings Controls
        draw_string(x, content_y, 0x38BDF8, "Screen Resolution & Graphics Mode:");
        draw_rect(x + 10, content_y + 18, 105, 22, (active_res == 1) ? theme->accent_color : 0x1E293B);
        draw_string(x + 15, content_y + 22, (active_res == 1) ? 0xFFFFFF : 0x94A3B8, "1024x768 HD");

        draw_rect(x + 120, content_y + 18, 105, 22, (active_res == 2) ? theme->accent_color : 0x1E293B);
        draw_string(x + 125, content_y + 22, (active_res == 2) ? 0xFFFFFF : 0x94A3B8, "1280x720 720p");

        draw_rect(x + 230, content_y + 18, 105, 22, (active_res == 3) ? theme->accent_color : 0x1E293B);
        draw_string(x + 235, content_y + 22, (active_res == 3) ? 0xFFFFFF : 0x94A3B8, "1920x1080 FHD");

        draw_rect(x + 340, content_y + 18, 105, 22, (active_res == 4) ? theme->accent_color : 0x1E293B);
        draw_string(x + 345, content_y + 22, (active_res == 4) ? 0xFFFFFF : 0x94A3B8, "800x600 SVGA");

        // Refresh Rate & Frame Pacing Target
        draw_string(x, content_y + 46, 0x38BDF8, "Refresh Rate & Frame Pacing Target:");
        draw_rect(x + 10, content_y + 64, 80, 22, (active_fps == 1) ? 0x10B981 : 0x1E293B);
        draw_string(x + 18, content_y + 68, (active_fps == 1) ? 0xFFFFFF : 0x94A3B8, "30 Hz");

        draw_rect(x + 95, content_y + 64, 80, 22, (active_fps == 2) ? 0x10B981 : 0x1E293B);
        draw_string(x + 103, content_y + 68, (active_fps == 2) ? 0xFFFFFF : 0x94A3B8, "60 Hz");

        draw_rect(x + 180, content_y + 64, 80, 22, (active_fps == 3) ? 0x10B981 : 0x1E293B);
        draw_string(x + 188, content_y + 68, (active_fps == 3) ? 0xFFFFFF : 0x94A3B8, "90 Hz");

        draw_rect(x + 265, content_y + 64, 80, 22, (active_fps == 4) ? 0x10B981 : 0x1E293B);
        draw_string(x + 273, content_y + 68, (active_fps == 4) ? 0xFFFFFF : 0x94A3B8, "120 Hz");

        draw_rect(x + 350, content_y + 64, 95, 22, (active_fps == 5) ? 0x10B981 : 0x1E293B);
        draw_string(x + 356, content_y + 68, (active_fps == 5) ? 0xFFFFFF : 0x94A3B8, "Uncapped");

        // Brightness & Night Light Filter
        draw_string(x, content_y + 92, 0x38BDF8, "Screen Brightness Level & Night Light:");
        draw_rect(x + 10, content_y + 110, 75, 22, (brightness_level == 100) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 18, content_y + 114, (brightness_level == 100) ? 0xFFFFFF : 0x94A3B8, "100%");

        draw_rect(x + 90, content_y + 110, 75, 22, (brightness_level == 85) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 98, content_y + 114, (brightness_level == 85) ? 0xFFFFFF : 0x94A3B8, "85%");

        draw_rect(x + 170, content_y + 110, 75, 22, (brightness_level == 70) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 178, content_y + 114, (brightness_level == 70) ? 0xFFFFFF : 0x94A3B8, "70%");

        draw_rect(x + 250, content_y + 110, 75, 22, (brightness_level == 50) ? 0xF59E0B : 0x1E293B);
        draw_string(x + 258, content_y + 114, (brightness_level == 50) ? 0xFFFFFF : 0x94A3B8, "50%");

        draw_rect(x + 330, content_y + 110, 115, 22, (night_light == 1) ? 0xEC4899 : 0x1E293B);
        draw_string(x + 336, content_y + 114, (night_light == 1) ? 0xFFFFFF : 0x94A3B8, night_light ? "[X] Warm 4500K" : "[ ] Night Light");

        // Color Profile & VSync Pacing
        draw_string(x, content_y + 138, 0x38BDF8, "Display Color Profile & Frame Pacing:");
        draw_rect(x + 10, content_y + 156, 115, 22, (color_profile == 1) ? 0x8B5CF6 : 0x1E293B);
        draw_string(x + 16, content_y + 160, (color_profile == 1) ? 0xFFFFFF : 0x94A3B8, "sRGB Standard");

        draw_rect(x + 130, content_y + 156, 115, 22, (color_profile == 2) ? 0x8B5CF6 : 0x1E293B);
        draw_string(x + 136, content_y + 160, (color_profile == 2) ? 0xFFFFFF : 0x94A3B8, "DCI-P3 Vivid");

        draw_rect(x + 250, content_y + 156, 110, 22, (vsync_enabled == 1) ? 0x10B981 : 0x1E293B);
        draw_string(x + 256, content_y + 160, (vsync_enabled == 1) ? 0xFFFFFF : 0x94A3B8, vsync_enabled ? "VSync: ON" : "VSync: OFF");

        draw_string(x + 10, content_y + 195, 0x4ADE80, settings_msg);
    }
    else if (active_tab == 2) {
        // Tab 2: Personalization & Themes
        draw_string(x, content_y, 0x38BDF8, "System-Wide Color Themes (Updates Desktop, Taskbar & Windows):");

        draw_rect(x + 10, content_y + 20, 110, 26, (active_theme == 1) ? theme->accent_color : 0x1E293B);
        draw_string(x + 18, content_y + 25, (active_theme == 1) ? 0xFFFFFF : 0x94A3B8, "1. Midnight");

        draw_rect(x + 125, content_y + 20, 110, 26, (active_theme == 2) ? theme->accent_color : 0x1E293B);
        draw_string(x + 133, content_y + 25, (active_theme == 2) ? 0xFFFFFF : 0x94A3B8, "2. Cyber");

        draw_rect(x + 240, content_y + 20, 110, 26, (active_theme == 3) ? theme->accent_color : 0x1E293B);
        draw_string(x + 248, content_y + 25, (active_theme == 3) ? 0xFFFFFF : 0x94A3B8, "3. Emerald");

        draw_rect(x + 355, content_y + 20, 110, 26, (active_theme == 4) ? theme->accent_color : 0x1E293B);
        draw_string(x + 363, content_y + 25, (active_theme == 4) ? 0xFFFFFF : 0x94A3B8, "4. Purple");

        draw_rect(x + 10, content_y + 54, 110, 26, (active_theme == 5) ? theme->accent_color : 0x1E293B);
        draw_string(x + 18, content_y + 59, (active_theme == 5) ? 0xFFFFFF : 0x94A3B8, "5. Synthwave");

        draw_string(x, content_y + 94, 0x38BDF8, "Window Manager Accent & Controls:");
        draw_string(x + 10, content_y + 114, 0x4ADE80, "[X] macOS Style Red/Yellow/Green Traffic Control Dots");
        draw_string(x + 10, content_y + 130, 0x4ADE80, "[X] Active Window Glowing Border Accent");
        draw_string(x + 10, content_y + 146, 0x4ADE80, "[X] Translucent Launcher & Start Panel");
        draw_string(x + 10, content_y + 172, 0x38BDF8, settings_msg);
    }
    else if (active_tab == 3) {
        // Tab 3: Mouse
        draw_string(x, content_y, 0x38BDF8, "Mouse Pointer Calibration & Direction:");
        draw_string(x + 10, content_y + 20, 0x4ADE80, "Axis Orientation: CORRECTED (Up = Up, Down = Down)");
        draw_string(x + 10, content_y + 36, 0x94A3B8, "PS/2 Packet Sanity Filtering: ACTIVE (Max Delta Clamp: +/- 45px)");

        draw_string(x, content_y + 70, 0x38BDF8, "VirtualBox Pointer Integration Status:");
        draw_string(x + 10, content_y + 90, 0x38BDF8, "Edge Boundary Damping: ACTIVE (Prevents Screen Escapes)");
        draw_string(x + 10, content_y + 106, 0x94A3B8, "VMMDev Absolute Protocol: ACTIVE (Port 0x5040)");
    }
    else if (active_tab == 4) {
        // Tab 4: EXT4 Storage
        draw_string(x, content_y, 0x38BDF8, "EXT4 Storage & File System Inspection:");

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
        draw_string(x + 10, content_y + 86, 0xFFFFFF, "Graphics: 32-bit Linear Framebuffer + Dynamic Window Manager");
        draw_string(x + 10, content_y + 115, 0x94A3B8, "Falkon-OS Enterprise Operating System Project.");
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
        // Resolution clicks
        if (rel_y >= content_y + 18 && rel_y <= content_y + 40) {
            if (rel_x >= 10 && rel_x <= 115) { active_res = 1; graphics_set_resolution(1024, 768); }
            else if (rel_x >= 120 && rel_x <= 225) { active_res = 2; graphics_set_resolution(1280, 720); }
            else if (rel_x >= 230 && rel_x <= 335) { active_res = 3; graphics_set_resolution(1920, 1080); }
            else if (rel_x >= 340 && rel_x <= 445) { active_res = 4; graphics_set_resolution(800, 600); }
            taskbar_init(); // Reposition taskbar for new screen size
            strcpy(settings_msg, "Resolution applied & display buffer updated.");
        }
        // Refresh Rate / FPS Target clicks
        else if (rel_y >= content_y + 64 && rel_y <= content_y + 86) {
            if (rel_x >= 10 && rel_x <= 90) { active_fps = 1; graphics_set_fps_target(1); }
            else if (rel_x >= 95 && rel_x <= 175) { active_fps = 2; graphics_set_fps_target(2); }
            else if (rel_x >= 180 && rel_x <= 260) { active_fps = 3; graphics_set_fps_target(3); }
            else if (rel_x >= 265 && rel_x <= 345) { active_fps = 4; graphics_set_fps_target(4); }
            else if (rel_x >= 350 && rel_x <= 445) { active_fps = 5; graphics_set_fps_target(5); }
            strcpy(settings_msg, "Refresh rate pacing target updated.");
        }
        // Brightness & Night Light clicks
        else if (rel_y >= content_y + 110 && rel_y <= content_y + 132) {
            if (rel_x >= 10 && rel_x <= 85) { brightness_level = 100; graphics_set_brightness(100); }
            else if (rel_x >= 90 && rel_x <= 165) { brightness_level = 85; graphics_set_brightness(85); }
            else if (rel_x >= 170 && rel_x <= 245) { brightness_level = 70; graphics_set_brightness(70); }
            else if (rel_x >= 250 && rel_x <= 325) { brightness_level = 50; graphics_set_brightness(50); }
            else if (rel_x >= 330 && rel_x <= 445) { night_light = !night_light; graphics_set_night_light(night_light); }
            strcpy(settings_msg, "Screen brightness / night light updated.");
        }
        // Color Profiles & VSync
        else if (rel_y >= content_y + 156 && rel_y <= content_y + 178) {
            if (rel_x >= 10 && rel_x <= 125) color_profile = 1;
            else if (rel_x >= 130 && rel_x <= 245) color_profile = 2;
            else if (rel_x >= 250 && rel_x <= 360) vsync_enabled = !vsync_enabled;
            strcpy(settings_msg, "Color profile & VSync setting updated.");
        }
        settings_redraw(win);
    }
    else if (active_tab == 2) {
        // Theme selection clicks - Updates Desktop, Taskbar, and Windows Globally!
        if (rel_y >= content_y + 20 && rel_y <= content_y + 46) {
            if (rel_x >= 10 && rel_x <= 120) { active_theme = 1; desktop_set_theme(1); }
            else if (rel_x >= 125 && rel_x <= 235) { active_theme = 2; desktop_set_theme(2); }
            else if (rel_x >= 240 && rel_x <= 350) { active_theme = 3; desktop_set_theme(3); }
            else if (rel_x >= 355 && rel_x <= 465) { active_theme = 4; desktop_set_theme(4); }
            strcpy(settings_msg, "System-Wide Theme Applied Globally!");
        } else if (rel_y >= content_y + 54 && rel_y <= content_y + 80) {
            if (rel_x >= 10 && rel_x <= 120) { active_theme = 5; desktop_set_theme(5); }
            strcpy(settings_msg, "Synthwave System Theme Applied!");
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
            strcpy(settings_msg, "EXT4 Partition Formatted & Mounted Globally!");
            settings_redraw(win);
        }
    }
}

void settings_open(void) {
    window_t* win = wm_create_window(90, 60, 490, 310, "Display & System Settings");
    if (win) {
        win->render_content = settings_redraw;
        win->on_click = settings_handle_click;
        taskbar_add_button(win->id, "Settings");
        settings_redraw(win);
    }
}
