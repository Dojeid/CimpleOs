#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stddef.h>

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SEC_COUNT    0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMMAND      0x1F7
#define ATA_PRIMARY_STATUS       0x1F7

#define ATA_CMD_READ_SECTORS     0x20
#define ATA_CMD_WRITE_SECTORS    0x30
#define ATA_CMD_IDENTIFY         0xEC

typedef struct {
    uint8_t present;
    uint32_t total_sectors;
    char model[41];
} ata_drive_t;

void ata_init(void);
ata_drive_t* ata_get_drive(uint8_t drive_num);
int ata_read_sectors(uint32_t lba, uint8_t count, uint8_t* buffer);
int ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t* buffer);

#endif // ATA_H
