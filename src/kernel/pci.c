#include "pci.h"
#include "io.h"

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                       (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                       (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

int pci_find_device(uint8_t class_code, uint8_t subclass, uint8_t prog_if, struct pci_device* out) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor = pci_read_config(bus, slot, 0, 0);
            if ((vendor & 0xFFFF) == 0xFFFF) continue;
            
            uint32_t class_info = pci_read_config(bus, slot, 0, 0x08);
            uint8_t dev_class = (class_info >> 24) & 0xFF;
            uint8_t dev_subclass = (class_info >> 16) & 0xFF;
            uint8_t dev_prog_if = (class_info >> 8) & 0xFF;
            
            if (dev_class == class_code && dev_subclass == subclass && dev_prog_if == prog_if) {
                out->bus = bus;
                out->slot = slot;
                out->func = 0;
                out->vendor_id = vendor & 0xFFFF;
                out->device_id = (vendor >> 16) & 0xFFFF;
                out->class_code = dev_class;
                out->subclass = dev_subclass;
                out->prog_if = dev_prog_if;
                out->bar0 = pci_read_config(bus, slot, 0, 0x10) & 0xFFFFFFF0;
                out->bar1 = pci_read_config(bus, slot, 0, 0x14) & 0xFFFFFFF0;
                uint32_t irq_info = pci_read_config(bus, slot, 0, 0x3C);
                out->interrupt_line = irq_info & 0xFF;
                return 1;
            }
        }
    }
    return 0;
}
