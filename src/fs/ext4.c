#include "fs/ext4.h"
#include "drivers/storage/ata.h"
#include "lib/string.h"
#include "drivers/video/vga.h"
#include "fs/vfs.h"

static ext4_superblock_t mounted_sb;
static int is_mounted = 0;
static uint32_t mounted_lba = 0;

int ext4_format_drive(uint32_t start_lba, uint32_t total_sectors, const char* volume_name) {
    if (total_sectors < 4096) return -1; // Require at least 2MB partition

    vga_print("[EXT4] Formatting partition starting at LBA ");
    vga_print("...\n");

    uint8_t sector_buf[512];
    memset(sector_buf, 0, 512);

    // 1. Write Superblock at LBA start_lba + 2 (1024 bytes offset from partition start)
    ext4_superblock_t sb;
    memset(&sb, 0, sizeof(sb));

    sb.s_inodes_count = 1024;
    sb.s_blocks_count_lo = total_sectors / 2; // 1024-byte blocks
    sb.s_free_blocks_count_lo = (total_sectors / 2) - 100;
    sb.s_free_inodes_count = 1020;
    sb.s_first_data_block = 1;
    sb.s_log_block_size = 0; // 1024 bytes per block
    sb.s_blocks_per_group = 8192;
    sb.s_inodes_per_group = 1024;
    sb.s_magic = EXT4_SUPER_MAGIC; // 0xEF53
    sb.s_state = 1; // Clean
    sb.s_rev_level = 1;
    sb.s_inode_size = 128;
    sb.s_first_ino = 11;
    strncpy(sb.s_volume_name, volume_name ? volume_name : "FalkonExt4", 15);

    // Write superblock (1024 bytes = 2 sectors at start_lba + 2)
    memcpy(sector_buf, &sb, sizeof(sb));
    if (ata_write_sectors(start_lba + 2, 1, sector_buf) != 0) return -1;

    // 2. Write Block Group Descriptor at start_lba + 4
    ext4_group_desc_t gd;
    memset(&gd, 0, sizeof(gd));
    gd.bg_block_bitmap_lo = 3;
    gd.bg_inode_bitmap_lo = 4;
    gd.bg_inode_table_lo = 5;
    gd.bg_free_blocks_count_lo = sb.s_free_blocks_count_lo;
    gd.bg_free_inodes_count_lo = sb.s_free_inodes_count;
    gd.bg_used_dirs_count_lo = 1;

    memset(sector_buf, 0, 512);
    memcpy(sector_buf, &gd, sizeof(gd));
    if (ata_write_sectors(start_lba + 4, 1, sector_buf) != 0) return -1;

    // 3. Write Root Inode (Inode 2 = Root directory)
    ext4_inode_t root_ino;
    memset(&root_ino, 0, sizeof(root_ino));
    root_ino.i_mode = EXT4_S_IFDIR | 0755;
    root_ino.i_size_lo = 1024;
    root_ino.i_links_count = 2;
    root_ino.i_blocks_lo = 2;
    root_ino.i_block[0] = 10; // Root directory data block

    memset(sector_buf, 0, 512);
    memcpy(sector_buf + (128 * 1), &root_ino, sizeof(root_ino)); // Inode 2 (index 1)
    if (ata_write_sectors(start_lba + 10, 1, sector_buf) != 0) return -1;

    vga_print("[EXT4] Filesystem formatted successfully with 0xEF53 magic.\n");
    return 0;
}

int ext4_mount_drive(uint32_t start_lba) {
    uint8_t sector_buf[512];
    if (ata_read_sectors(start_lba + 2, 1, sector_buf) != 0) return -1;

    memcpy(&mounted_sb, sector_buf, sizeof(ext4_superblock_t));

    if (mounted_sb.s_magic != EXT4_SUPER_MAGIC) {
        vga_print("[EXT4] Superblock magic mismatch. Partition is not EXT4.\n");
        is_mounted = 0;
        return -1;
    }

    mounted_lba = start_lba;
    is_mounted = 1;

    vga_print("[EXT4] Mounted volume '");
    vga_print(mounted_sb.s_volume_name);
    vga_print("' successfully.\n");
    return 0;
}

int ext4_is_mounted(void) {
    return is_mounted;
}

ext4_superblock_t* ext4_get_superblock(void) {
    return is_mounted ? &mounted_sb : 0;
}
