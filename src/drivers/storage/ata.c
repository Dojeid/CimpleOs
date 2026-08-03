#include "drivers/storage/ata.h"
#include "lib/io.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

static ata_drive_t primary_master = {0, 0, ""};

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
extern void outw(uint16_t port, uint16_t val);
extern uint16_t inw(uint16_t port);

// Returns 0 on success, ATA_ERR_TIMEOUT if drive never clears BSY
static int ata_wait_bsy(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if (!(inb(ATA_PRIMARY_STATUS) & 0x80)) return ATA_OK;
    }
    return ATA_ERR_TIMEOUT;
}

// Returns 0 on success, ATA_ERR_TIMEOUT if DRQ never sets
static int ata_wait_drq(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        uint8_t s = inb(ATA_PRIMARY_STATUS);
        if (s & 0x01) return ATA_ERR_STATUS;  // ERR bit
        if (s & 0x08) return ATA_OK;           // DRQ bit
    }
    return ATA_ERR_TIMEOUT;
}

void ata_init(void) {
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    outb(ATA_PRIMARY_SEC_COUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HIGH, 0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        vga_print("[ATA] Primary Master drive not detected.\n");
        primary_master.present = 0;
        return;
    }

    if (ata_wait_bsy() != ATA_OK) {
        vga_print("[ATA] Timeout waiting for BSY clear.\n");
        primary_master.present = 0;
        return;
    }
    status = inb(ATA_PRIMARY_STATUS);
    if (status & 0x01) { // ERR bit set
        vga_print("[ATA] Primary Master identify error.\n");
        primary_master.present = 0;
        return;
    }

    if (ata_wait_drq() != ATA_OK) {
        vga_print("[ATA] Timeout waiting for DRQ.\n");
        primary_master.present = 0;
        return;
    }

    uint16_t data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = inw(ATA_PRIMARY_DATA);
    }

    primary_master.present = 1;
    primary_master.total_sectors = ((uint32_t)data[61] << 16) | data[60];
    if (primary_master.total_sectors == 0) {
        primary_master.total_sectors = 2097152; // Default 1GB virtual disk fallback
    }

    // Model name extraction
    for (int i = 0; i < 20; i++) {
        primary_master.model[i * 2] = (char)(data[27 + i] >> 8);
        primary_master.model[i * 2 + 1] = (char)(data[27 + i] & 0xFF);
    }
    primary_master.model[40] = '\0';

    vga_print("[ATA] Primary Master Drive Detected (");
    vga_print(primary_master.model);
    vga_print(").\n");
}

ata_drive_t* ata_get_drive(uint8_t drive_num) {
    (void)drive_num;
    return &primary_master;
}

int ata_read_sectors(uint32_t lba, uint8_t count, uint8_t* buffer) {
    if (!buffer || count == 0) return ATA_ERR_NODRIVE;
    if (!primary_master.present)  return ATA_ERR_NODRIVE;

    if (ata_wait_bsy() != ATA_OK) return ATA_ERR_TIMEOUT;

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SEC_COUNT,  count);
    outb(ATA_PRIMARY_LBA_LOW,    (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID,    (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH,   (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND,    ATA_CMD_READ_SECTORS);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int s = 0; s < count; s++) {
        if (ata_wait_bsy() != ATA_OK) return ATA_ERR_TIMEOUT;
        if (ata_wait_drq() != ATA_OK) return ATA_ERR_STATUS;
        for (int i = 0; i < 256; i++)
            ptr[s * 256 + i] = inw(ATA_PRIMARY_DATA);
    }
    return ATA_OK;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t* buffer) {
    if (!buffer || count == 0) return ATA_ERR_NODRIVE;
    if (!primary_master.present)  return ATA_ERR_NODRIVE;

    if (ata_wait_bsy() != ATA_OK) return ATA_ERR_TIMEOUT;

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SEC_COUNT,  count);
    outb(ATA_PRIMARY_LBA_LOW,    (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID,    (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH,   (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND,    ATA_CMD_WRITE_SECTORS);

    const uint16_t* ptr = (const uint16_t*)buffer;
    for (int s = 0; s < count; s++) {
        if (ata_wait_bsy() != ATA_OK) return ATA_ERR_TIMEOUT;
        if (ata_wait_drq() != ATA_OK) return ATA_ERR_STATUS;
        for (int i = 0; i < 256; i++)
            outw(ATA_PRIMARY_DATA, ptr[s * 256 + i]);
    }

    // Commit write buffer to physical disk platters
    return ata_cache_flush();
}

// Flush ATA drive write cache to persistent storage (cmd 0xE7)
int ata_cache_flush(void) {
    if (!primary_master.present) return ATA_OK; // no drive, nothing to flush
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0);         // select master
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
    return ata_wait_bsy();                       // wait until flush done
}
