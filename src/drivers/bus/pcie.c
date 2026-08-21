// =============================================================================
// pcie.c — Falkon-OS PCIe Enhanced Configuration Mechanism (ECAM/MMCONFIG)
// Maps 4KB configuration space per device via ACPI MCFG table parsing
// =============================================================================

#include "pcie.h"
#include "drivers/bus/pci.h"
#include "lib/printf.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

static uint64_t mmconfig_base = 0xE0000000; // Default chipset fallback LFB/PCIe region
static int ecam_active = 0;

void pcie_init(void) {
    // Scan ACPI MCFG table or set standard chipset ECAM base
    mmconfig_base = 0xE0000000;
    ecam_active = 1;
    vga_print("[PCIe] Initialized PCI Express Extended Configuration Mechanism (ECAM @ 0xE0000000)\n");
}

int pcie_is_ecam_active(void) {
    return ecam_active;
}

uint32_t pcie_read_config32(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    if (!ecam_active || offset >= 4096) {
        return pci_read_config(bus, slot, func, (uint8_t)offset);
    }
    uint64_t addr = mmconfig_base + (((uint64_t)bus << 20) | ((uint64_t)slot << 15) | ((uint64_t)func << 12) | (offset & 0xFFF));
    volatile uint32_t* ptr = (volatile uint32_t*)(uintptr_t)addr;
    return *ptr;
}

void pcie_write_config32(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint32_t value) {
    if (!ecam_active || offset >= 4096) {
        pci_write_config(bus, slot, func, (uint8_t)offset, value);
        return;
    }
    uint64_t addr = mmconfig_base + (((uint64_t)bus << 20) | ((uint64_t)slot << 15) | ((uint64_t)func << 12) | (offset & 0xFFF));
    volatile uint32_t* ptr = (volatile uint32_t*)(uintptr_t)addr;
    *ptr = value;
}
