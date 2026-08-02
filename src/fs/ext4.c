#include "fs/ext4.h"
#include "drivers/storage/ata.h"
#include "lib/string.h"
#include "drivers/video/vga.h"
#include "fs/vfs.h"
#include "mm/heap.h"

static ext4_superblock_t mounted_sb;
static int is_mounted = 0;
static uint32_t mounted_lba = 0;

static int ext4_vfs_read(file_t *file, uint32_t offset, uint32_t size, uint8_t *buffer) {
    vga_print("[EXT4] vfs_read stub\n");
    return 0; // Not implemented yet
}

static file_operations_t ext4_fops = {
    .read = ext4_vfs_read,
    .write = 0,
    .open = 0,
    .release = 0
};

static dentry_t* ext4_vfs_lookup(inode_t *dir, const char *name) {
    vga_print("[EXT4] lookup stub for: ");
    vga_print(name);
    vga_print("\n");
    return 0;
}

static inode_operations_t ext4_iops = {
    .lookup = ext4_vfs_lookup,
    .mkdir = 0,
    .create = 0
};

static super_block_t* ext4_vfs_mount(file_system_type_t *fs_type, const char *dev_name) {
    if (!ext4_is_mounted()) {
        // Hardcode mounting LBA 0 for now
        if (ext4_mount_drive(0) != 0) return 0;
    }

    super_block_t *sb = (super_block_t*)kmalloc(sizeof(super_block_t));
    memset(sb, 0, sizeof(super_block_t));
    sb->s_type = fs_type;
    sb->s_fs_info = &mounted_sb;

    // Create root inode
    inode_t *root_inode = (inode_t*)kmalloc(sizeof(inode_t));
    memset(root_inode, 0, sizeof(inode_t));
    root_inode->i_ino = 2; // EXT4 Root Inode is 2
    root_inode->i_mode = EXT4_S_IFDIR | 0755;
    root_inode->i_sb = sb;
    root_inode->i_op = &ext4_iops;
    root_inode->i_fop = &ext4_fops;

    // Create root dentry
    dentry_t *root_dentry = (dentry_t*)kmalloc(sizeof(dentry_t));
    memset(root_dentry, 0, sizeof(dentry_t));
    strcpy(root_dentry->d_name, "/");
    root_dentry->d_inode = root_inode;
    root_dentry->d_sb = sb;

    sb->s_root = root_dentry;
    return sb;
}

static file_system_type_t ext4_fs_type = {
    .name = "ext4",
    .mount = ext4_vfs_mount,
    .next = 0
};

void ext4_init(void) {
    register_filesystem(&ext4_fs_type);
}

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
