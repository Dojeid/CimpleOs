#include "fs/fat32.h"
#include "drivers/storage/ata.h"
#include "lib/string.h"
#include "lib/printf.h"

static fat32_bpb_t bpb;
static int mounted = 0;
static uint32_t fat_lba = 0;
static uint32_t data_lba = 0;
static uint32_t root_cluster = 0;
static uint32_t partition_lba_start = 0;

int fat32_init(void) {
    mounted = 0;
    return 0;
}

int fat32_mount(uint32_t lba_start) {
    partition_lba_start = lba_start;
    uint8_t sector_buf[512];
    
    // Read the boot sector (first sector of the partition)
    if (ata_read_sectors(lba_start, 1, sector_buf) != 0) {
        return -1;
    }
    
    memcpy(&bpb, sector_buf, sizeof(fat32_bpb_t));
    
    // Basic verification to ensure we have somewhat valid data
    if (bpb.bytes_per_sector != 512) {
        return -1; // We only support 512 bytes per sector here
    }
    
    fat_lba = lba_start + bpb.reserved_sectors;
    
    // Calculate root directory sectors (will be 0 for FAT32, but handled for clarity)
    uint32_t root_dir_sectors = ((bpb.root_entry_count * 32) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector;
    
    uint32_t fat_size = bpb.sectors_per_fat_16;
    if (fat_size == 0) {
        fat_size = bpb.sectors_per_fat_32;
    }
    
    data_lba = fat_lba + (bpb.fat_count * fat_size) + root_dir_sectors;
    
    if (bpb.root_cluster == 0) {
        // FAT16 default root cluster emulation
        root_cluster = 2;
    } else {
        root_cluster = bpb.root_cluster;
    }
    
    mounted = 1;
    return 0;
}

static uint32_t cluster_to_lba(uint32_t cluster) {
    if (cluster < 2) return data_lba;
    return data_lba + ((cluster - 2) * bpb.sectors_per_cluster);
}

static uint32_t fat32_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_lba + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    
    uint8_t sector_buf[512];
    if (ata_read_sectors(fat_sector, 1, sector_buf) != 0) {
        return 0x0FFFFFFF;
    }
    
    uint32_t next_cluster = *((uint32_t*)&sector_buf[ent_offset]) & 0x0FFFFFFF;
    return next_cluster;
}

static int fat32_read_cluster(uint32_t cluster, uint8_t* buf) {
    uint32_t lba = cluster_to_lba(cluster);
    return ata_read_sectors(lba, bpb.sectors_per_cluster, buf);
}

static void parse_83_name(char* out_name, const char* in_name) {
    int out_idx = 0;
    for (int i = 0; i < 8; i++) {
        if (in_name[i] != ' ') {
            out_name[out_idx++] = in_name[i];
        }
    }
    if (in_name[8] != ' ') {
        out_name[out_idx++] = '.';
        for (int i = 8; i < 11; i++) {
            if (in_name[i] != ' ') {
                out_name[out_idx++] = in_name[i];
            }
        }
    }
    out_name[out_idx] = '\0';
}

int fat32_list_dir(const char* path, char* out_buf, uint32_t max_len) {
    if (!mounted) return -1;
    
    uint32_t cluster = root_cluster;
    uint8_t cluster_buf[32768]; // Max typical cluster size to avoid over-allocating on stack
    
    if ((uint32_t)bpb.sectors_per_cluster * 512 > sizeof(cluster_buf)) return -1;
    
    out_buf[0] = '\0';
    uint32_t buf_len = 0;
    
    while (cluster < 0x0FFFFFF8) {
        if (fat32_read_cluster(cluster, cluster_buf) != 0) return -1;
        
        for (uint32_t i = 0; i < ((uint32_t)bpb.sectors_per_cluster * 512); i += 32) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)&cluster_buf[i];
            
            if (entry->name[0] == 0x00) break; // End of directory
            if (entry->name[0] == 0xE5) continue; // Deleted entry
            if (entry->attributes == FAT32_ATTR_LFN) continue; // Skip LFN for now
            if (entry->attributes & FAT32_ATTR_VOLUME_ID) continue; // Skip volume label
            
            char filename[13];
            parse_83_name(filename, entry->name);
            
            int len = strlen(filename);
            if (buf_len + len + 2 < max_len) {
                strcat(out_buf, filename);
                strcat(out_buf, "\n");
                buf_len += len + 1;
            }
        }
        
        cluster = fat32_next_cluster(cluster);
    }
    
    return 0;
}

static int fat32_find_entry(uint32_t dir_cluster, const char* name, fat32_dir_entry_t* out_entry) {
    uint32_t cluster = dir_cluster;
    uint8_t cluster_buf[32768];
    
    while (cluster < 0x0FFFFFF8) {
        if (fat32_read_cluster(cluster, cluster_buf) != 0) return -1;
        
        for (uint32_t i = 0; i < ((uint32_t)bpb.sectors_per_cluster * 512); i += 32) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)&cluster_buf[i];
            
            if (entry->name[0] == 0x00) return -1; 
            if (entry->name[0] == 0xE5) continue;
            if (entry->attributes == FAT32_ATTR_LFN) continue;
            
            char filename[13];
            parse_83_name(filename, entry->name);
            
            if (strcmp(filename, name) == 0) {
                memcpy(out_entry, entry, sizeof(fat32_dir_entry_t));
                return 0;
            }
        }
        
        cluster = fat32_next_cluster(cluster);
    }
    return -1;
}

int fat32_read_file(const char* path, uint8_t* buf, uint32_t max_len) {
    if (!mounted) return -1;
    
    const char* filename = path;
    if (path[0] == '/') filename++; 
    
    fat32_dir_entry_t entry;
    if (fat32_find_entry(root_cluster, filename, &entry) != 0) {
        return -1;
    }
    
    uint32_t cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    uint32_t bytes_read = 0;
    uint8_t cluster_buf[32768];
    
    while (cluster < 0x0FFFFFF8 && bytes_read < max_len && bytes_read < entry.file_size) {
        if (fat32_read_cluster(cluster, cluster_buf) != 0) return -1;
        
        uint32_t to_copy = bpb.sectors_per_cluster * 512;
        if (bytes_read + to_copy > entry.file_size) {
            to_copy = entry.file_size - bytes_read;
        }
        if (bytes_read + to_copy > max_len) {
            to_copy = max_len - bytes_read;
        }
        
        memcpy(buf + bytes_read, cluster_buf, to_copy);
        bytes_read += to_copy;
        
        cluster = fat32_next_cluster(cluster);
    }
    
    return (int)bytes_read;
}

void fat32_unmount(void) {
    mounted = 0;
}

int fat32_is_mounted(void) {
    return mounted;
}
