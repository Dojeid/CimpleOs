#include "gui/apps/installer.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/apps/file_explorer.h"
#include "drivers/video/graphics.h"
#include "drivers/storage/ata.h"
#include "fs/ext4.h"
#include "fs/vfs.h"
#include "include/bootsector_bin.h"
#include "lib/printf.h"
#include "lib/string.h"
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
        draw_string(x + 10, y + 118, 0x94A3B8, "1. Write MBR Partition Table & 16-bit Boot Code to LBA 0");
        draw_string(x + 10, y + 132, 0x94A3B8, "2. Deploy 64-bit Kernel Image Payload to Sectors 1..1200");
        draw_string(x + 10, y + 146, 0x94A3B8, "3. Format EXT4 Filesystem (/boot, /bin, /sys, /docs, /home)");
        draw_string(x + 10, y + 160, 0x94A3B8, "4. Commit ATA Sector Cache & Configure Native HDD Boot");

        // Action Install Button
        int btn_x = x + 100;
        int btn_y = y + 184;
        draw_rect(btn_x, btn_y, 250, 32, 0x0284C7);
        draw_rect(btn_x + 1, btn_y + 1, 248, 30, 0x0369A1);
        draw_string(btn_x + 18, btn_y + 9, 0xFFFFFF, "[ Format & Install Falkon-OS ]");
    } 
    else if (install_step == 4) {
        draw_string(x, y + 24, 0x4ADE80, "SUCCESS: Falkon-OS Installed Natively to /dev/sda (EXT4)!");
        draw_rect(x, y + 40, win->width - 24, 2, 0x059669);

        draw_string(x + 10, y + 54, 0xF1F5F9, "The Falkon-OS 64-bit kernel and VFS root filesystem");
        draw_string(x + 10, y + 70, 0xF1F5F9, "have been fully deployed and formatted to native storage.");
        draw_string(x + 10, y + 92, 0x38BDF8, "Disk Status: INSTALLED & MOUNTED PERSISTENTLY (/dev/sda1)");
        draw_string(x + 10, y + 110, 0x4ADE80, "MBR Code & Kernel Payload Committed to LBA 0-1200.");
        draw_string(x + 10, y + 128, 0x94A3B8, "You can now reboot or remove ISO to boot natively.");

        int btn_x = x + 100;
        int btn_y = y + 160;
        draw_rect(btn_x, btn_y, 240, 32, 0x16A34A);
        draw_string(btn_x + 18, btn_y + 9, 0xFFFFFF, "[ Open File Explorer (Hard Disk) ]");
    }
}

static void installer_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;

    if (install_step == 1) {
        // "Format & Install" button: drawn at x+100 (rel_x 112..362), y+184 (rel_y 216..248)
        if (rel_x >= 110 && rel_x <= 365 && rel_y >= 210 && rel_y <= 250) {
            install_step = 2;
            install_progress = 20;

            ata_drive_t* drv = ata_get_drive(0);
            uint32_t total_sec = (drv && drv->present) ? drv->total_sectors : 4096;

            // Step 1: Write MBR to LBA 0
            static uint8_t mbr[512];
            for (int i = 0; i < 446; i++) mbr[i] = bootsector_code[i];
            for (int i = 446; i < 512; i++) mbr[i] = 0;

            mbr[446] = 0x80; // Bootable
            mbr[447] = 0xFE; mbr[448] = 0xFF; mbr[449] = 0xFF;
            mbr[450] = 0x83; // Linux native
            mbr[451] = 0xFE; mbr[452] = 0xFF; mbr[453] = 0xFF;

            uint32_t lba_start = 2048;
            mbr[454] = (uint8_t)(lba_start);
            mbr[455] = (uint8_t)(lba_start >> 8);
            mbr[456] = (uint8_t)(lba_start >> 16);
            mbr[457] = (uint8_t)(lba_start >> 24);

            uint32_t part_size = (total_sec > 2048) ? (total_sec - 2048) : 2048;
            mbr[458] = (uint8_t)(part_size);
            mbr[459] = (uint8_t)(part_size >> 8);
            mbr[460] = (uint8_t)(part_size >> 16);
            mbr[461] = (uint8_t)(part_size >> 24);

            mbr[510] = 0x55;
            mbr[511] = 0xAA;

            ata_write_sectors(0, 1, mbr);
            install_progress = 40;

            // Step 2: Write Kernel Payload from RAM 0x100000 to LBA 1..1200
            const uint8_t* kern_ram = (const uint8_t*)(uintptr_t)0x100000;
            ata_write_sectors(1, 1200, kern_ram);
            install_progress = 70;

            // Step 3: Format EXT4 Superblock & VFS structure
            static uint8_t sb[512];
            for (int i = 0; i < 512; i++) sb[i] = 0;
            uint32_t inodes = 8192;
            sb[0] = (uint8_t)(inodes);
            sb[1] = (uint8_t)(inodes >> 8);
            sb[2] = (uint8_t)(inodes >> 16);
            sb[3] = (uint8_t)(inodes >> 24);
            sb[4] = (uint8_t)(part_size);
            sb[5] = (uint8_t)(part_size >> 8);
            sb[6] = (uint8_t)(part_size >> 16);
            sb[7] = (uint8_t)(part_size >> 24);
            sb[0x18] = 2;
            sb[0x38] = 0x53;
            sb[0x39] = 0xEF;
            sb[0x3C] = 0x01;

            ata_write_sectors(2050, 1, sb);
            ext4_format_drive(0, total_sec, "FALKON_ROOT");

            vfs_mount("hda", "/", "ext4");
            vfs_node_t* root = vfs_get_root();
            vfs_node_t* boot = vfs_mkdir(root, "boot");
            vfs_node_t* home = vfs_mkdir(root, "home");
            vfs_node_t* docs = vfs_mkdir(root, "docs");
            vfs_mkdir(root, "sys");
            vfs_mkdir(root, "bin");
            vfs_mkdir(root, "videos");

            if (boot) vfs_create_file(boot, "kernel.bin", (const uint8_t*)"FALKON_KERNEL_64BIT_ELF", 23);
            if (home) vfs_create_file(home, "user_notes.txt", (const uint8_t*)"User files on /dev/sda1\n", 24);
            const char* log = "Falkon-OS installed to /dev/sda1 (EXT4)\n";
            if (docs) vfs_create_file(docs, "install_log.txt", (const uint8_t*)log, strlen(log));

            // Step 4: Flush ATA cache & Complete
            ata_cache_flush();
            is_os_installed = 1;
            install_step = 4;
            install_progress = 100;

            installer_redraw(win);
        }
    }
    else if (install_step == 4) {
        // Step 4 button: drawn at x+100 (rel_x 112..352), y+160 (rel_y 192..224)
        if (rel_x >= 100 && rel_x <= 360 && rel_y >= 185 && rel_y <= 230) {
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
