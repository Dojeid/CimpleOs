#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AHCI_PCI_CLASS    0x01
#define AHCI_PCI_SUBCLASS 0x06

typedef struct {
    uint32_t base_addr;
    int      ports_active;
    int      drive_present;
    uint64_t sector_count;
} ahci_device_t;

int            ahci_init(void);
ahci_device_t* ahci_get_device(void);
int            ahci_read_sectors(uint64_t lba, uint32_t count, uint8_t* buf);
int            ahci_write_sectors(uint64_t lba, uint32_t count, const uint8_t* buf);

#ifdef __cplusplus
}
#endif

#endif // AHCI_H
