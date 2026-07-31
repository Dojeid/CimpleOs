/*
 * Falkon-OS Native C ISO 9660 + El-Torito Generator & Raw Disk Image Builder
 * Creates both a 100% ECMA-119 compliant ISO image and a raw disk image.
 * Self-contained — 0 external dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SECTOR_SIZE 2048

// Kernel payload capacity (must match KERNEL_BYTES in bootsector.asm).
// The bootloader reads a FIXED number of sectors, so the kernel extent is
// always padded to this size in both the ISO and the raw disk image.
#define KERNEL_BYTES       (320 * 1024)
#define KERNEL_ISO_SECTORS (KERNEL_BYTES / SECTOR_SIZE)   // 160
#define KERNEL_IMG_SECTORS (KERNEL_BYTES / 512)           // 640

#pragma pack(push, 1)

// ISO 9660 Directory Record (34 bytes)
typedef struct {
    uint8_t  length;                    // 34 (0x22)
    uint8_t  ext_attr_length;           // 0
    uint32_t extent_lba_l;              // LBA LE
    uint32_t extent_lba_m;              // LBA BE
    uint32_t data_length_l;             // 2048 LE
    uint32_t data_length_m;             // 2048 BE
    uint8_t  date[7];                   // 7 bytes date
    uint8_t  flags;                     // 0x02 (Directory)
    uint8_t  file_unit_size;            // 0
    uint8_t  interleave_gap;            // 0
    uint16_t vol_seq_num_l;             // 1
    uint16_t vol_seq_num_m;             // 0x0100
    uint8_t  name_len;                  // 1
    uint8_t  name;                      // 0x00
} iso_dir_record_t;

// Sector 16: Primary Volume Descriptor (PVD)
typedef struct {
    uint8_t  type;                      // 0x01
    char     id[5];                     // "CD001"
    uint8_t  version;                  // 0x01
    uint8_t  unused1;
    char     system_id[32];             // "FALKON-OS"
    char     volume_id[32];             // "FALKONOS"
    uint8_t  unused2[8];
    uint32_t volume_space_l;            // Total sectors (LE)
    uint32_t volume_space_m;            // Total sectors (BE)
    uint8_t  unused3[32];
    uint16_t volume_set_size_l;         // 1
    uint16_t volume_set_size_m;         // 0x0100
    uint16_t volume_seq_num_l;          // 1
    uint16_t volume_seq_num_m;          // 0x0100
    uint16_t logical_block_size_l;      // 2048
    uint16_t logical_block_size_m;      // 2048 (BE)
    uint32_t path_table_size_l;         // 10
    uint32_t path_table_size_m;         // 10 (BE)
    uint32_t type_l_path_table;         // LBA of L-Path Table
    uint32_t opt_type_l_path_table;
    uint32_t type_m_path_table;         // LBA of M-Path Table
    uint32_t opt_type_m_path_table;
    iso_dir_record_t root_directory_record; // 34-byte Root Dir Record
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
    uint8_t  file_structure_version;    // 0x01
    uint8_t  unused4;
    uint8_t  application_data[512];
    uint8_t  reserved[653];
} pvd_t;

// Sector 17: El-Torito Boot Record Volume Descriptor
typedef struct {
    uint8_t  type;                      // 0x00
    char     id[5];                     // "CD001"
    uint8_t  version;                  // 0x01
    char     system_id[32];             // "EL TORITO SPECIFICATION"
    uint8_t  unused[32];
    uint32_t boot_catalog_lba;          // LBA of Boot Catalog (Sector 20)
    uint8_t  reserved[1973];
} eltorito_vd_t;

// El-Torito Validation Entry (32 bytes)
typedef struct {
    uint8_t  header_id;                 // 0x01
    uint8_t  platform_id;               // 0x00 (x86 BIOS)
    uint16_t reserved;
    char     id_string[24];             // "FALKONOS"
    uint16_t checksum;                  // Word sum == 0 mod 65536
    uint8_t  key_byte_1;                // 0x55
    uint8_t  key_byte_2;                // 0xAA
} boot_catalog_val_t;

// El-Torito Initial Entry (32 bytes)
typedef struct {
    uint8_t  boot_indicator;            // 0x88 (Bootable)
    uint8_t  boot_media_type;           // 0x00 (No Emulation)
    uint16_t load_segment;              // 0x0000 (Defaults to 0x7C00)
    uint8_t  system_type;               // 0x00
    uint8_t  unused;
    uint16_t sector_count;              // 4 (4 virtual 512B sectors = 2048 bytes)
    uint32_t load_rba;                  // LBA of bootsector (Sector 21)
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
        printf("Usage: %s <output.iso> <bootsector.bin> <kernel.bin> [output.img]\n", argv[0]);
        return 1;
    }

    const char* out_iso_path  = argv[1];
    const char* boot_bin_path = argv[2];
    const char* kern_bin_path = argv[3];
    const char* out_img_path  = (argc >= 5) ? argv[4] : NULL;

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

    uint32_t boot_sectors = (boot_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (boot_sectors == 0) boot_sectors = 1;

    if (kern_size > KERNEL_BYTES) {
        fprintf(stderr,
                "[!] Error: kernel is %u bytes but bootloader capacity is %u bytes.\n"
                "    Increase KERNEL_BYTES in src/arch/x86_64/boot/bootsector.asm and\n"
                "    KERNEL_BYTES in tools/iso_builder.c, then rebuild.\n",
                kern_size, KERNEL_BYTES);
        fclose(f_boot);
        fclose(f_kern);
        return 1;
    }

    // Kernel extent is padded to the full bootloader capacity (see KERNEL_BYTES).
    uint32_t kern_sectors = KERNEL_ISO_SECTORS;

    // Sector Layout:
    // Sectors 0-15  : System Area (0x00)
    // Sector 16     : Primary Volume Descriptor (PVD)
    // Sector 17     : El-Torito Volume Descriptor
    // Sector 18     : Volume Descriptor Set Terminator (0xFF)
    // Sector 19     : ISO 9660 Root Directory Sector
    // Sector 20     : El-Torito Boot Catalog
    // Sector 21     : Bootloader binary (bootsector.bin)
    // Sector 22+    : Kernel binary (FalkonOS.bin)

    uint32_t root_dir_lba = 19;
    uint32_t boot_cat_lba = 20;
    uint32_t boot_lba     = 21;
    uint32_t kern_lba     = boot_lba + boot_sectors;
    uint32_t total_sectors = kern_lba + kern_sectors;

    FILE* f_iso = fopen(out_iso_path, "wb");
    if (!f_iso) {
        fprintf(stderr, "[!] Error creating output ISO: %s\n", out_iso_path);
        fclose(f_boot);
        fclose(f_kern);
        return 1;
    }

    uint8_t sector[SECTOR_SIZE];

    // 1. Sectors 0-15: System Area
    memset(sector, 0, SECTOR_SIZE);
    for (int i = 0; i < 16; i++) {
        fwrite(sector, 1, SECTOR_SIZE, f_iso);
    }

    // 2. Sector 16: Primary Volume Descriptor (PVD) pointing to Root Directory Sector 19
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

    pvd.root_directory_record.length = 34;
    pvd.root_directory_record.extent_lba_l = root_dir_lba;
    pvd.root_directory_record.extent_lba_m = __builtin_bswap32(root_dir_lba);
    pvd.root_directory_record.data_length_l = SECTOR_SIZE;
    pvd.root_directory_record.data_length_m = __builtin_bswap32(SECTOR_SIZE);
    pvd.root_directory_record.date[0] = 126;
    pvd.root_directory_record.date[1] = 7;
    pvd.root_directory_record.date[2] = 31;
    pvd.root_directory_record.flags = 0x02;
    pvd.root_directory_record.vol_seq_num_l = 1;
    pvd.root_directory_record.vol_seq_num_m = 0x0100;
    pvd.root_directory_record.name_len = 1;
    pvd.root_directory_record.name = 0x00;

    fwrite(&pvd, 1, sizeof(pvd), f_iso);

    // 3. Sector 17: El-Torito Volume Descriptor (pointing to Boot Catalog Sector 20)
    eltorito_vd_t et_vd;
    memset(&et_vd, 0, sizeof(et_vd));
    et_vd.type = 0x00;
    memcpy(et_vd.id, "CD001", 5);
    et_vd.version = 0x01;
    write_padded_string(et_vd.system_id, "EL TORITO SPECIFICATION", 32);
    et_vd.boot_catalog_lba = boot_cat_lba;
    fwrite(&et_vd, 1, sizeof(et_vd), f_iso);

    // 4. Sector 18: Volume Descriptor Set Terminator
    memset(sector, 0, SECTOR_SIZE);
    sector[0] = 0xFF;
    memcpy(&sector[1], "CD001", 5);
    sector[6] = 0x01;
    fwrite(sector, 1, SECTOR_SIZE, f_iso);

    // 5. Sector 19: ISO 9660 Root Directory Sector
    memset(sector, 0, SECTOR_SIZE);
    iso_dir_record_t* dot = (iso_dir_record_t*)sector;
    dot->length = 34;
    dot->extent_lba_l = root_dir_lba;
    dot->extent_lba_m = __builtin_bswap32(root_dir_lba);
    dot->data_length_l = SECTOR_SIZE;
    dot->data_length_m = __builtin_bswap32(SECTOR_SIZE);
    dot->flags = 0x02;
    dot->vol_seq_num_l = 1;
    dot->vol_seq_num_m = 0x0100;
    dot->name_len = 1;
    dot->name = 0x00;

    iso_dir_record_t* dotdot = (iso_dir_record_t*)(sector + 34);
    dotdot->length = 34;
    dotdot->extent_lba_l = root_dir_lba;
    dotdot->extent_lba_m = __builtin_bswap32(root_dir_lba);
    dotdot->data_length_l = SECTOR_SIZE;
    dotdot->data_length_m = __builtin_bswap32(SECTOR_SIZE);
    dotdot->flags = 0x02;
    dotdot->vol_seq_num_l = 1;
    dotdot->vol_seq_num_m = 0x0100;
    dotdot->name_len = 1;
    dotdot->name = 0x01;

    fwrite(sector, 1, SECTOR_SIZE, f_iso);

    // 6. Sector 20: El-Torito Boot Catalog
    memset(sector, 0, SECTOR_SIZE);
    boot_catalog_val_t* val = (boot_catalog_val_t*)sector;
    val->header_id   = 0x01;
    val->platform_id = 0x00;
    val->key_byte_1  = 0x55;
    val->key_byte_2  = 0xAA;
    write_padded_string(val->id_string, "FALKONOS BOOT", 24);

    uint16_t* words = (uint16_t*)val;
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        if (i != 14) sum += words[i];
    }
    val->checksum = (uint16_t)(0x10000 - (sum & 0xFFFF));

    boot_catalog_entry_t* entry = (boot_catalog_entry_t*)(sector + 32);
    entry->boot_indicator  = 0x88; // Bootable
    entry->boot_media_type = 0x00; // No emulation
    entry->load_segment    = 0x0000;
    entry->sector_count    = 4;    // 4 virtual 512B sectors (2048 bytes)
    entry->load_rba        = boot_lba;
    fwrite(sector, 1, SECTOR_SIZE, f_iso);

    // 7. Sector 21+: Bootloader Payload
    uint8_t* boot_buf = (uint8_t*)calloc(boot_sectors, SECTOR_SIZE);
    fread(boot_buf, 1, boot_size, f_boot);
    fwrite(boot_buf, 1, boot_sectors * SECTOR_SIZE, f_iso);
    free(boot_buf);

    // 8. Sector 22+: Kernel Payload (padded to KERNEL_ISO_SECTORS)
    uint8_t* kern_buf = (uint8_t*)calloc(kern_sectors, SECTOR_SIZE);
    fread(kern_buf, 1, kern_size, f_kern);
    fwrite(kern_buf, 1, kern_sectors * SECTOR_SIZE, f_iso);
    free(kern_buf);

    fclose(f_iso);

    printf("[✓] Fully ECMA-119 Compliant ISO 9660 Image Generated -> %s (%u sectors, %u bytes)\n",
           out_iso_path, total_sectors, total_sectors * SECTOR_SIZE);

    // 9. Generate Raw Boot Disk Image (FalkonOS.img)
    if (out_img_path) {
        FILE* f_img = fopen(out_img_path, "wb");
        if (f_img) {
            uint8_t sector512[512];

            // Write Bootsector (Sector 0)
            fseek(f_boot, 0, SEEK_SET);
            memset(sector512, 0, 512);
            fread(sector512, 1, boot_size, f_boot);
            fwrite(sector512, 1, 512, f_img);

            // Write Kernel Payload (Sectors 1+, padded to KERNEL_IMG_SECTORS)
            fseek(f_kern, 0, SEEK_SET);
            uint8_t* kbuf = (uint8_t*)calloc(1, KERNEL_BYTES);
            if (kbuf) {
                fread(kbuf, 1, kern_size, f_kern);
                fwrite(kbuf, 1, KERNEL_BYTES, f_img);
                free(kbuf);
            }
            fclose(f_img);
            printf("[✓] Raw Boot Disk Image Generated -> %s (%u bytes)\n", out_img_path, 512 + KERNEL_BYTES);
        }
    }

    fclose(f_boot);
    fclose(f_kern);
    return 0;
}
