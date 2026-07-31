#include "gui/apps/installer.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "fs/ext4.h"
#include "lib/string.h"
#include "lib/printf.h"

static int install_step = 1; // 1=Scan/Welcome, 2=Formatting, 3=Copying, 4=Complete
static int install_progress = 0;

static void installer_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;

    // Dark slate container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x111827);

    // Title header
    draw_string(x, y, 0x38BDF8, "Falkon-OS Enterprise Disk Installer Wizard");
    draw_rect(x, y + 16, win->width - 16, 2, 0x374151);

    ata_drive_t* drive = ata_get_drive(0);

    if (install_step == 1) {
        draw_string(x, y + 26, 0xF1F5F9, "Target Installation Hard Disk Drive:");
        
        draw_rect(x, y + 44, win->width - 16, 50, 0x1F2937);
        draw_rect(x, y + 44, win->width - 16, 1, 0x38BDF8);

        char dev_info[128];
        sprintf(dev_info, "/dev/sda: %s (%u MB)", drive->present ? drive->model : "Virtual Storage Disk", (drive->total_sectors * 512) / (1024 * 1024));
        draw_string(x + 12, y + 54, 0x4ADE80, dev_info);
        draw_string(x + 12, y + 70, 0x94A3B8, "Target Filesystem: EXT4 (Native Linux EXT4 - 0xEF53)");

        draw_string(x, y + 104, 0xE2E8F0, "Installation Summary:");
        draw_string(x + 10, y + 122, 0x94A3B8, "1. Write MBR Partition Table (Type 0x83 EXT4 Partition)");
        draw_string(x + 10, y + 138, 0x94A3B8, "2. Format EXT4 Superblock, Inode Table & Block Groups");
        draw_string(x + 10, y + 154, 0x94A3B8, "3. Copy 64-bit Kernel, /bin, /docs, /sys, and Desktop UI");
        draw_string(x + 10, y + 170, 0x94A3B8, "4. Install Stage1 MBR Bootloader to LBA Sector 0");

        // Action Button: Begin Install
        draw_rect(x + 120, y + 200, 200, 32, 0x2563EB);
        draw_string(x + 140, y + 210, 0xFFFFFF, "[ Install Falkon-OS ]");
    } 
    else if (install_step == 2 || install_step == 3) {
        draw_string(x, y + 30, 0xF1F5F9, install_step == 2 ? "Formatting EXT4 Partition (/dev/sda1)..." : "Copying Falkon System Payload to EXT4...");
        
        // Progress Bar Outer Box
        int bar_w = win->width - 40;
        draw_rect(x + 10, y + 60, bar_w, 24, 0x1F2937);
        draw_rect(x + 10, y + 60, bar_w, 1, 0x374151);

        // Progress Bar Fill
        int fill_w = (bar_w * install_progress) / 100;
        if (fill_w > bar_w) fill_w = bar_w;
        draw_rect(x + 10, y + 60, fill_w, 24, 0x10B981);

        char pct_str[32];
        sprintf(pct_str, "%u%% Complete", install_progress);
        draw_string(x + (bar_w / 2) - 30, y + 66, 0xFFFFFF, pct_str);

        if (install_step == 2) {
            draw_string(x + 10, y + 100, 0x94A3B8, "> Writing EXT4 Superblock (Magic 0xEF53)");
            draw_string(x + 10, y + 116, 0x94A3B8, "> Creating Block Group Descriptor Table & Inodes");
        } else {
            draw_string(x + 10, y + 100, 0x4ADE80, "> Copying 64-bit Kernel Payload to /boot/vmlinuz-falkon");
            draw_string(x + 10, y + 116, 0x4ADE80, "> Deploying VFS Root Filesystem (/docs, /sys, /bin)");
        }
    }
    else if (install_step == 4) {
        draw_string(x, y + 30, 0x4ADE80, "SUCCESS: Falkon-OS Installed to /dev/sda (EXT4)!");
        draw_rect(x, y + 46, win->width - 16, 2, 0x059669);

        draw_string(x + 10, y + 60, 0xF1F5F9, "The operating system payload and stage1 bootloader");
        draw_string(x + 10, y + 76, 0xF1F5F9, "have been written to your target hard drive.");
        draw_string(x + 10, y + 100, 0x38BDF8, "You can now boot directly from your EXT4 disk!");

        draw_rect(x + 140, y + 160, 160, 32, 0x059669);
        draw_string(x + 160, y + 170, 0xFFFFFF, "[ Reboot OS ]");
    }
}

void installer_open(void) {
    window_t* win = wm_create_window(140, 70, 460, 290, "OS Installer");
    if (win) {
        win->render_content = installer_redraw;
        taskbar_add_button(win->id, "Installer");
        installer_redraw(win);
    }
}
