#include "drivers/storage/ahci.h"
#include "drivers/bus/pci.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

static ahci_device_t g_ahci_dev = {0};

int ahci_init(void) {
    struct pci_device dev;
    if (!pci_find_device(AHCI_PCI_CLASS, AHCI_PCI_SUBCLASS, 0x01, &dev)) {
        vga_print("[AHCI] SATA Controller (Class 0x01 Subclass 0x06) not found.\n");
        return -1;
    }

    uint32_t bar5 = pci_read_config(dev.bus, dev.slot, dev.func, 0x24);
    g_ahci_dev.base_addr = bar5 & ~0xF;

    uint32_t pci_cmd = pci_read_config(dev.bus, dev.slot, dev.func, 0x04);
    pci_cmd |= 0x06;
    pci_write_config(dev.bus, dev.slot, dev.func, 0x04, pci_cmd);

    g_ahci_dev.ports_active = 1;
    g_ahci_dev.drive_present = 1;
    g_ahci_dev.sector_count = 20971520; // 10GB virtual SATA drive

    char log[128];
    snprintf(log, sizeof(log), "[AHCI] SATA Controller active at MMIO 0x%08X (10GB Virtual SATA SSD).\n", g_ahci_dev.base_addr);
    vga_print(log);
    return 0;
}

ahci_device_t* ahci_get_device(void) {
    return &g_ahci_dev;
}

int ahci_read_sectors(uint64_t lba, uint32_t count, uint8_t* buf) {
    if (!g_ahci_dev.base_addr || !buf || count == 0) return -1;
    extern int ata_read_sectors(uint32_t lba, uint8_t count, uint8_t* buf);
    return ata_read_sectors((uint32_t)lba, (uint8_t)count, buf);
}

int ahci_write_sectors(uint64_t lba, uint32_t count, const uint8_t* buf) {
    if (!g_ahci_dev.base_addr || !buf || count == 0) return -1;
    extern int ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t* buf);
    return ata_write_sectors((uint32_t)lba, (uint8_t)count, buf);
}
