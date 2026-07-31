/*
 * Falkon-OS Native C ISO 9660 + El-Torito Generator
 * Creates a bootable ISO image containing custom bootloader and kernel binaries.
 * Completely self-contained — 0 external dependencies (no xorriso, no mkisofs).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SECTOR_SIZE 2048

#pragma pack(push, 1)

// Sector 16: Primary Volume Descriptor
typedef struct {
    uint8_t  type;                     // 0x01
    char     id[5];                    // "CD001"
    uint8_t  version;                 // 0x01
    uint8_t  unused1;
    char     system_id[32];            // "FALKON-OS"
    char     volume_id[32];            // "FALKONOS"
    uint8_t  unused2[8];
    uint32_t volume_space_l;           // Total sectors (LE)
    uint32_t volume_space_m;           // Total sectors (BE)
    uint8_t  unused3[32];
    uint16_t volume_set_size_l;
    uint16_t volume_set_size_m;
    uint16_t volume_seq_num_l;
    uint16_t volume_seq_num_m;
    uint16_t logical_block_size_l;     // 2048
    uint16_t logical_block_size_m;     // 2048
    uint32_t path_table_size_l;
    uint32_t path_table_size_m;
    uint32_t type_l_path_table;
    uint32_t opt_type_l_path_table;
    uint32_t type_m_path_table;
    uint32_t opt_type_m_path_table;
    uint8_t  root_directory_record[34];
    char     volume_set_id[128];
    char     publisher_id[128];
    char     data_preparer_id[128];
    char     application_id[128];
    char     copyright_file_id[37];
    char     abstract_file_id[37];
    char     bibliographic_file_id[37];
    char     creation_date[17];
    char     modification_date[17];
    char     expiration_date[17];
    char     effective_date[17];
    uint8_t  file_structure_version;   // 0x01
    uint8_t  unused4;
    uint8_t  application_data[512];
    uint8_t  reserved[653];
} pvd_t;

// Sector 17: El-Torito Boot Record Volume Descriptor
typedef struct {
    uint8_t  type;                    // 0x00
    char     id[5];                   // "CD001"
    uint8_t  version;                // 0x01
    char     system_id[32];           // "EL TORITO SPECIFICATION"
    uint8_t  unused[32];
    uint32_t boot_catalog_lba;        // Pointer to sector 19
    uint8_t  reserved[1973];
} eltorito_vd_t;

// El-Torito Validation Entry (32 bytes)
typedef struct {
    uint8_t  header_id;               // 0x01
    uint8_t  platform_id;             // 0x00 (x86 BIOS)
    uint16_t reserved;
    char     id_string[24];
    uint16_t checksum;
    uint8_t  key_byte_1;              // 0x55
    uint8_t  key_byte_2;              // 0xAA
} boot_catalog_val_t;

// El-Torito Initial / Default Entry (32 bytes)
typedef struct {
    uint8_t  boot_indicator;          // 0x88 (Bootable)
    uint8_t  boot_media_type;         // 0x00 (No Emulation)
    uint16_t load_segment;            // 0x0000 (Defaults to 0x7C00)
    uint8_t  system_type;             // 0x00
    uint8_t  unused;
    uint16_t sector_count;            // Virtual 512-byte sectors to load
    uint32_t load_rba;                // Start sector LBA
    uint8_t  unused2[20];
} boot_catalog_entry_t;

#pragma pack(pop)

static void write_padded_string(char* dst, const char* src, size_t len) {
    memset(dst, ' ', len);
    size_t slen = strlen(src);
    if (slen > len) slen = len;
    memcpy(dst, src, slen);
}

static uint32_t get_file_size(FILE* f) {
    fseek(f, 0, SEEK_END);
    uint32_t sz = (uint32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    return sz;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <output.iso> <bootsector.bin> <kernel.bin>\n", argv[0]);
        return 1;
    }

    const char* out_iso_path  = argv[1];
    const char* boot_bin_path = argv[2];
    const char* kern_bin_path = argv[3];

    FILE* f_boot = fopen(boot_bin_path, "rb");
    if (!f_boot) {
        fprintf(stderr, "[!] Error opening bootsector binary: %s\n", boot_bin_path);
        return 1;
    }
    uint32_t boot_size = get_file_size(f_boot);

    FILE* f_kern = fopen(kern_bin_path, "rb");
    if (!f_kern) {
        fprintf(stderr, "[!] Error opening kernel binary: %s\n", kern_bin_path);
        fclose(f_boot);
        return 1;
    }
    uint32_t kern_size = get_file_size(f_kern);

    // Calculate sectors
    uint32_t boot_sectors = (boot_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint32_t kern_sectors = (kern_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (boot_sectors == 0) boot_sectors = 1;

    // Sector Layout:
    // Sectors 0-15  : System Area (0x00)
    // Sector 16     : Primary Volume Descriptor (PVD)
    // Sector 17     : El-Torito Volume Descriptor
    // Sector 18     : Volume Descriptor Set Terminator (0xFF)
    // Sector 19     : El-Torito Boot Catalog
    // Sector 20     : Bootloader binary payload (bootsector.bin)
    // Sector 20+N   : Kernel binary payload (FalkonOS.bin)

    uint32_t boot_lba = 20;
    uint32_t kern_lba = boot_lba + boot_sectors;
    uint32_t total_sectors = kern_lba + kern_sectors;

    FILE* f_iso = fopen(out_iso_path, "wb");
    if (!f_iso) {
        fprintf(stderr, "[!] Error creating output ISO: %s\n", out_iso_path);
        fclose(f_boot);
        fclose(f_kern);
        return 1;
    }

    uint8_t sector[SECTOR_SIZE];

    // --- Sectors 0 to 15: Zero-filled System Area ---
    memset(sector, 0, SECTOR_SIZE);
    for (int i = 0; i < 16; i++) {
        fwrite(sector, 1, SECTOR_SIZE, f_iso);
    }

    // --- Sector 16: Primary Volume Descriptor ---
    pvd_t pvd;
    memset(&pvd, 0, sizeof(pvd));
    pvd.type = 0x01;
    memcpy(pvd.id, "CD001", 5);
    pvd.version = 0x01;
    write_padded_string(pvd.system_id, "FALKON-OS", 32);
    write_padded_string(pvd.volume_id, "FALKONOS", 32);
    pvd.volume_space_l = total_sectors;
    pvd.volume_space_m = __builtin_bswap32(total_sectors);
    pvd.volume_set_size_l = 1;
    pvd.volume_set_size_m = 0x0100;
    pvd.volume_seq_num_l  = 1;
    pvd.volume_seq_num_m  = 0x0100;
    pvd.logical_block_size_l = SECTOR_SIZE;
    pvd.logical_block_size_m = __builtin_bswap16(SECTOR_SIZE);
    pvd.file_structure_version = 0x01;
    fwrite(&pvd, 1, sizeof(pvd), f_iso);

    // --- Sector 17: El Torito Boot Record Descriptor ---
    eltorito_vd_t et_vd;
    memset(&et_vd, 0, sizeof(et_vd));
    et_vd.type = 0x00;
    memcpy(et_vd.id, "CD001", 5);
    et_vd.version = 0x01;
    write_padded_string(et_vd.system_id, "EL TORITO SPECIFICATION", 32);
    et_vd.boot_catalog_lba = 19;
    fwrite(&et_vd, 1, sizeof(et_vd), f_iso);

    // --- Sector 18: Volume Descriptor Set Terminator ---
    memset(sector, 0, SECTOR_SIZE);
    sector[0] = 0xFF; // Type: Terminator
    memcpy(&sector[1], "CD001", 5);
    sector[6] = 0x01;
    fwrite(sector, 1, SECTOR_SIZE, f_iso);

    // --- Sector 19: Boot Catalog ---
    memset(sector, 0, SECTOR_SIZE);
    boot_catalog_val_t* val = (boot_catalog_val_t*)sector;
    val->header_id   = 0x01;
    val->platform_id = 0x00; // x86 BIOS
    val->key_byte_1  = 0x55;
    val->key_byte_2  = 0xAA;
    write_padded_string(val->id_string, "FALKONOS BOOT", 24);

    // Calculate validation checksum (sum of all 16-bit words in validation entry == 0)
    uint16_t* words = (uint16_t*)val;
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        if (i != 14) sum += words[i]; // Skip checksum field itself
    }
    val->checksum = (uint16_t)(0x10000 - (sum & 0xFFFF));

    boot_catalog_entry_t* entry = (boot_catalog_entry_t*)(sector + 32);
    entry->boot_indicator  = 0x88; // Bootable
    entry->boot_media_type = 0x00; // No emulation
    entry->load_segment    = 0x0000; // Load to default 0x7C00
    entry->sector_count    = 4;      // 4 virtual 512-byte sectors (1 full 2KB ISO sector)
    entry->load_rba        = boot_lba;
    fwrite(sector, 1, SECTOR_SIZE, f_iso);

    // --- Sector 20+: Bootloader Payload ---
    uint8_t* boot_buf = (uint8_t*)calloc(boot_sectors, SECTOR_SIZE);
    fread(boot_buf, 1, boot_size, f_boot);
    fwrite(boot_buf, 1, boot_sectors * SECTOR_SIZE, f_iso);
    free(boot_buf);
    fclose(f_boot);

    // --- Sector 20+N: Kernel Payload ---
    uint8_t* kern_buf = (uint8_t*)calloc(kern_sectors, SECTOR_SIZE);
    fread(kern_buf, 1, kern_size, f_kern);
    fwrite(kern_buf, 1, kern_sectors * SECTOR_SIZE, f_iso);
    free(kern_buf);
    fclose(f_kern);

    fclose(f_iso);

    printf("[✓] Native ISO Generation Successful: %s\n", out_iso_path);
    printf("    -> Bootloader (LBA %u, %u sectors)\n", boot_lba, boot_sectors);
    printf("    -> Kernel     (LBA %u, %u sectors, %u bytes)\n", kern_lba, kern_sectors, kern_size);

    return 0;
}
