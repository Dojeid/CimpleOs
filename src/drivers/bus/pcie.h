#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include <stddef.h>

// PCIe ECAM Address Base Structure from ACPI MCFG Table
struct mcfg_allocation {
    uint64_t base_address;
    uint16_t pci_segment;
    uint8_t  start_bus;
    uint8_t  end_bus;
    uint32_t reserved;
};

void     pcie_init(void);
uint32_t pcie_read_config32(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset);
void     pcie_write_config32(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint32_t value);
int      pcie_is_ecam_active(void);

#endif // PCIE_H
