#include "sysinfo.h"
#include "kernel/cpuid.h"
#include "mm/pmm.h"
#include "drivers/bus/pci.h"
#include "lib/printf.h"

void sysinfo_init() {
    // Initialize CPUID
    cpuid_init();
}

void sysinfo_print() {
    printf("\n=== SYSTEM INFORMATION ===\n\n");
    
    // CPU Information
    printf("CPU:\n");
    printf("  Model: %s\n", cpu_info.brand);
    printf("  Vendor: %s\n", cpu_info.vendor);
    printf("  Family: %u, Model: %u, Stepping: %u\n", 
           cpu_info.family, cpu_info.model, cpu_info.stepping);
    
    cpuid_print_features(cpu_info.features_edx, cpu_info.features_ecx);
    
    if (cpu_info.logical_cores > 0) {
        printf("  Logical Cores: %u\n", cpu_info.logical_cores);
    }
    
    printf("\n");
    
    // Memory Information
    printf("Memory:\n");
    uint32_t total_mem = pmm_get_total_memory() / 1024 / 1024;  // MB
    uint32_t free_mem = pmm_get_free_memory() / 1024 / 1024;    // MB
    uint32_t used_mem = total_mem - free_mem;
    
    printf("  Total: %u MB\n", total_mem);
    printf("  Used: %u MB\n", used_mem);
    printf("  Free: %u MB\n", free_mem);
    
    printf("\n");
    
    // PCI Devices (optimized to scan only first 8 buses)
    printf("PCI Devices:\n");
    
    // Scan PCI bus (limit to 8 buses for performance)
    for (uint16_t bus = 0; bus < 8; bus++) {
        int empty_slots = 0;
        
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor = pci_read_config(bus, slot, 0, 0);
            
            if ((vendor & 0xFFFF) == 0xFFFF) {
                empty_slots++;
                // If we hit 8 empty slots in a row, skip rest of bus
                if (empty_slots > 8) break;
                continue;
            }
            
            empty_slots = 0;  // Reset counter
            
            uint16_t vendor_id = vendor & 0xFFFF;
            uint16_t device_id = (vendor >> 16) & 0xFFFF;
            
            uint32_t class_info = pci_read_config(bus, slot, 0, 0x08);
            uint8_t class_code = (class_info >> 24) & 0xFF;
            uint8_t subclass = (class_info >> 16) & 0xFF;
            
            // Determine device type name
            const char* type_name = "Unknown";
            if (class_code == 0x00) type_name = "Legacy Device";
            else if (class_code == 0x01) type_name = "Mass Storage";
            else if (class_code == 0x02) type_name = "Network Controller";
            else if (class_code == 0x03) type_name = "Display Controller";
            else if (class_code == 0x04) type_name = "Multimedia";
            else if (class_code == 0x05) type_name = "Memory Controller";
            else if (class_code == 0x06) type_name = "Bridge Device";
            else if (class_code == 0x0C) {
                if (subclass == 0x03) type_name = "USB Controller";
                else type_name = "Serial Bus";
            }
            
            printf("  %02x:%02x.0 %s [%04x:%04x]\n",
                   bus, slot, type_name, vendor_id, device_id);
        }
    }
    
    printf("\n=== END SYSTEM INFORMATION ===\n");
}
