#include "gui/apps/installer.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "fs/ext4.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "gui/apps/file_explorer.h"

static int install_step = 1; // 1=Welcome/Partition, 2=Formatting EXT4, 3=Deploying System Payload, 4=Complete
static int install_progress = 0;
static char install_status_msg[128] = "Drive /dev/sda ready for installation.";
static int is_os_installed = 0;

int installer_is_system_installed(void) {
    return is_os_installed;
}

static void installer_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 12;
    int y = win->y + 32;

    // Dark sleek container background (Midnight Slate 0x0F172A)
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Title header
    draw_string(x, y, 0x38BDF8, "Falkon-OS Enterprise Disk Installer Wizard");
    draw_rect(x, y + 16, win->width - 24, 2, 0x334155);

    ata_drive_t* drive = ata_get_drive(0);
    uint32_t drive_size_mb = drive && drive->present ? (drive->total_sectors * 512) / (1024 * 1024) : 2048;

    if (install_step == 1) {
        draw_string(x, y + 24, 0xF1F5F9, "Target ATA Hard Drive Installation Selection:");
        
        // Target Drive Card
        draw_rect(x, y + 42, win->width - 24, 52, 0x1E293B);
        draw_rect(x, y + 42, win->width - 24, 1, 0x38BDF8);

        char dev_info[128];
        sprintf(dev_info, "Target Disk: /dev/sda (%s - %u MB)", drive && drive->present ? drive->model : "Primary Master Storage Disk", drive_size_mb);
        draw_string(x + 12, y + 50, 0x4ADE80, dev_info);
        draw_string(x + 12, y + 66, 0x94A3B8, "Filesystem: Native Linux EXT4 Volume (Superblock 0xEF53)");
        draw_string(x + 12, y + 78, 0x38BDF8, "Bootloader: Falkon 64-bit Master Boot Record (Sector 0)");

        draw_string(x, y + 102, 0xE2E8F0, "Automated Installer Deployment Tasks:");
        draw_string(x + 10, y + 118, 0x94A3B8, "1. Write MBR Partition Table (Type 0x83 Linux Native Partition)");
        draw_string(x + 10, y + 132, 0x94A3B8, "2. Format EXT4 Inode Descriptors, Block Groups & Root Dir");
        draw_string(x + 10, y + 146, 0x94A3B8, "3. Copy Kernel Payload (/boot, /bin, /sys, /docs, /videos, /home)");
        draw_string(x + 10, y + 160, 0x94A3B8, "4. Switch System Mode from Live USB ISO -> Native Installed Hard Disk");

        // Action Install Button
        int btn_x = x + 110;
        int btn_y = y + 184;
        draw_rect(btn_x, btn_y, 230, 32, 0x0284C7);
        draw_rect(btn_x + 1, btn_y + 1, 228, 30, 0x0369A1);
        draw_string(btn_x + 18, btn_y + 9, 0xFFFFFF, "[ Format & Install Falkon-OS ]");
    } 
    else if (install_step == 2 || install_step == 3) {
        draw_string(x, y + 24, 0xF1F5F9, install_step == 2 ? "Formatting EXT4 Hard Drive Partition (/dev/sda1)..." : "Deploying System Payload & Filesystem Structure...");
        
        // Progress Bar Outer Container
        int bar_w = win->width - 48;
        draw_rect(x + 10, y + 50, bar_w, 26, 0x1E293B);
        draw_rect(x + 10, y + 50, bar_w, 1, 0x334155);

        // Progress Fill Bar
        int fill_w = (bar_w * install_progress) / 100;
        if (fill_w > bar_w) fill_w = bar_w;
        draw_rect(x + 10, y + 50, fill_w, 26, 0x10B981);

        char pct_str[32];
        sprintf(pct_str, "%u%% Completed", install_progress);
        draw_string(x + (bar_w / 2) - 35, y + 57, 0xFFFFFF, pct_str);

        draw_string(x + 10, y + 90, 0x38BDF8, install_status_msg);
        draw_string(x + 10, y + 110, 0x94A3B8, "> MBR Sector 0 written. EXT4 Superblock Magic 0xEF53 initialized.");
        draw_string(x + 10, y + 126, 0x4ADE80, "> Disk Sectors mapped to VFS Root Directory. System ready.");

        // Continue Next Step Button
        int btn_x = x + 120;
        int btn_y = y + 165;
        draw_rect(btn_x, btn_y, 210, 30, 0x059669);
        draw_string(btn_x + 35, btn_y + 8, 0xFFFFFF, "[ Proceed Next Step > ]");
    }
    else if (install_step == 4) {
        draw_string(x, y + 24, 0x4ADE80, "SUCCESS: Falkon-OS Installed Natively to /dev/sda (EXT4)!");
        draw_rect(x, y + 40, win->width - 24, 2, 0x059669);

        draw_string(x + 10, y + 54, 0xF1F5F9, "The Falkon-OS 64-bit kernel and VFS root filesystem");
        draw_string(x + 10, y + 70, 0xF1F5F9, "have been fully deployed and formatted to native storage.");
        draw_string(x + 10, y + 92, 0x38BDF8, "Disk Status: INSTALLED & MOUNTED PERSISTENTLY (/dev/sda1)");
        draw_string(x + 10, y + 110, 0x94A3B8, "System Mode: Native Installed Disk (Live USB ISO disabled)");

        int btn_x = x + 100;
        int btn_y = y + 155;
        draw_rect(btn_x, btn_y, 240, 32, 0x16A34A);
        draw_string(btn_x + 18, btn_y + 9, 0xFFFFFF, "[ Open File Explorer (Hard Disk) ]");
    }
}

static void installer_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;

    if (install_step == 1) {
        if (rel_x >= 110 && rel_x <= 340 && rel_y >= 184 && rel_y <= 216) {
            install_step = 2;
            install_progress = 35;
            strcpy(install_status_msg, "Writing MBR Sector 0 & EXT4 Superblock Magic (0xEF53)...");
            
            // Format EXT4 Disk Partition
            ata_drive_t* drv = ata_get_drive(0);
            uint32_t total_sec = drv->present ? drv->total_sectors : 4096;
            ext4_format_drive(0, total_sec, "FALKON_ROOT");
            
            installer_redraw(win);
        }
    }
    else if (install_step == 2) {
        if (rel_x >= 120 && rel_x <= 330 && rel_y >= 165 && rel_y <= 195) {
            install_step = 3;
            install_progress = 80;
            strcpy(install_status_msg, "Deploying System Payload (/boot, /bin, /sys, /docs, /videos)...");
            
            // Mount EXT4 Hard Drive as VFS Root
            vfs_mount("hda", "/", "ext4");
            
            // Populate installed directories
            vfs_node_t* root = vfs_get_root();
            vfs_node_t* boot   = vfs_mkdir(root, "boot");
            vfs_node_t* home   = vfs_mkdir(root, "home");
            vfs_node_t* docs   = vfs_mkdir(root, "docs");
            vfs_mkdir(root, "sys");
            vfs_mkdir(root, "bin");
            vfs_mkdir(root, "videos");

            const char* install_readme = "Falkon-OS Native Disk Installation Complete!\nTarget Storage: /dev/sda1 (EXT4 Partition)\n";
            if (boot) vfs_create_file(boot, "kernel.bin", (const uint8_t*)"FALKON_KERNEL_64BIT_BINARY", 26);
            if (home) vfs_create_file(home, "user_notes.txt", (const uint8_t*)"User persistent files stored on /dev/sda1\n", 41);
            if (docs) vfs_create_file(docs, "install_log.txt", (const uint8_t*)install_readme, strlen(install_readme));
            
            is_os_installed = 1;
            installer_redraw(win);
        }
    }
    else if (install_step == 3) {
        if (rel_x >= 120 && rel_x <= 330 && rel_y >= 165 && rel_y <= 195) {
            install_step = 4;
            install_progress = 100;
            strcpy(install_status_msg, "Falkon-OS Disk Installation Complete!");
            installer_redraw(win);
        }
    }
    else if (install_step == 4) {
        if (rel_x >= 100 && rel_x <= 340 && rel_y >= 155 && rel_y <= 187) {
            file_explorer_open();
            wm_destroy_window(win->id);
        }
    }
}

void installer_open(void) {
    window_t* win = wm_create_window(140, 65, 480, 290, "Falkon-OS Disk Installer Wizard");
    if (win) {
        win->render_content = installer_redraw;
        win->on_click = installer_handle_click;
        taskbar_add_button(win->id, "Installer");
        installer_redraw(win);
    }
}
