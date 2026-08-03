#include "kernel/acpi.h"
#include "lib/string.h"
#include "lib/printf.h"

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
extern void outw(uint16_t port, uint16_t val);
extern uint16_t inw(uint16_t port);
extern void terminal_print(const char* text);

static uint32_t smi_cmd_port = 0;
static uint8_t acpi_enable_cmd = 0;
static uint8_t acpi_disable_cmd = 0;
static uint32_t pm1a_cnt_blk = 0;
static uint32_t pm1b_cnt_blk = 0;
static uint16_t slp_typa = 0;
static uint16_t slp_typb = 0;
static int acpi_enabled_flag = 0;

static int verify_checksum(uint8_t* ptr, int length) {
    uint8_t sum = 0;
    for (int i = 0; i < length; i++) {
        sum += ptr[i];
    }
    return (sum == 0);
}

static void print_acpi_message(const char* msg) {
    terminal_print(msg);
}

static rsdp_t* find_rsdp_in_range(uint32_t start, uint32_t end) {
    for (uint32_t addr = start; addr < end; addr += 16) {
        if (memcmp((void*)(uintptr_t)addr, "RSD PTR ", 8) == 0) {
            rsdp_t* rsdp = (rsdp_t*)(uintptr_t)addr;
            if (verify_checksum((uint8_t*)rsdp, 20)) {
                return rsdp;
            }
        }
    }
    return NULL;
}

static rsdp_t* find_rsdp(void) {
    // Search EBDA
    uint16_t ebda_seg = *(uint16_t*)(0x40E);
    uint32_t ebda_start = ebda_seg << 4;
    rsdp_t* rsdp = NULL;
    
    if (ebda_start != 0) {
        rsdp = find_rsdp_in_range(ebda_start, ebda_start + 1024);
    }
    
    if (rsdp == NULL) {
        // Search main BIOS area
        rsdp = find_rsdp_in_range(0xE0000, 0xFFFFF);
    }
    
    return rsdp;
}

static void parse_fadt(fadt_t* fadt) {
    char buf[128];
    sprintf(buf, "[ACPI] FADT Parsing: PM1a_CNT_BLK=0x%X", fadt->pm1a_control_block);
    print_acpi_message(buf);
    
    smi_cmd_port = fadt->smi_command_port;
    acpi_enable_cmd = fadt->acpi_enable;
    acpi_disable_cmd = fadt->acpi_disable;
    pm1a_cnt_blk = fadt->pm1a_control_block;
    pm1b_cnt_blk = fadt->pm1b_control_block;
    
    // In a real scenario, DSDT/SSDT parsing would find the \_S5 object.
    // For many QEMU/Bochs VMs, SLP_TYPa is typically 5 or 0 for S5 state.
    // We will hardcode a fallback if we don't do full AML parsing.
    slp_typa = 5; 
    slp_typb = 5;
    
    // Try to enable ACPI if not already enabled (SCI_EN is bit 0)
    if ((inw(pm1a_cnt_blk) & 1) == 0) {
        if (smi_cmd_port != 0 && acpi_enable_cmd != 0) {
            print_acpi_message("[ACPI] Enabling ACPI mode via SMI command...");
            outb(smi_cmd_port, acpi_enable_cmd);
            
            // Wait up to 3 seconds for it to enable
            int timeout = 3000;
            while ((inw(pm1a_cnt_blk) & 1) == 0 && timeout > 0) {
                // busy wait
                for(volatile int k = 0; k < 10000; k++);
                timeout--;
            }
            if (inw(pm1a_cnt_blk) & 1) {
                print_acpi_message("[ACPI] ACPI enabled successfully.");
                acpi_enabled_flag = 1;
            } else {
                print_acpi_message("[ACPI] Error: Failed to enable ACPI.");
            }
        } else {
            print_acpi_message("[ACPI] Warning: No SMI port to enable ACPI.");
        }
    } else {
        print_acpi_message("[ACPI] ACPI is already enabled by BIOS/Bootloader.");
        acpi_enabled_flag = 1;
    }
}

void acpi_init(void) {
    print_acpi_message("[ACPI] Initializing Advanced Configuration and Power Interface...");
    
    rsdp_t* rsdp = find_rsdp();
    if (!rsdp) {
        print_acpi_message("[ACPI] Error: RSDP not found. ACPI is unsupported.");
        return;
    }
    
    char buf[128];
    sprintf(buf, "[ACPI] Found RSDP at 0x%X (Revision %d, OEM: %.6s)", 
            (uint32_t)(uintptr_t)rsdp, rsdp->revision, rsdp->oem_id);
    print_acpi_message(buf);
    
    rsdt_t* rsdt = (rsdt_t*)(uintptr_t)rsdp->rsdt_address;
    if (!verify_checksum((uint8_t*)rsdt, rsdt->header.length)) {
        print_acpi_message("[ACPI] Error: RSDT checksum invalid.");
        return;
    }
    
    int entries = (rsdt->header.length - sizeof(acpi_table_header_t)) / 4;
    sprintf(buf, "[ACPI] RSDT valid, length %d bytes, %d table entries.", rsdt->header.length, entries);
    print_acpi_message(buf);
    
    for (int i = 0; i < entries; i++) {
        acpi_table_header_t* table = (acpi_table_header_t*)(uintptr_t)rsdt->pointer_to_other_sdt[i];
        
        char sig[5] = {0};
        strncpy(sig, table->signature, 4);
        
        sprintf(buf, "[ACPI] Found Table: %s at 0x%X", sig, (uint32_t)(uintptr_t)table);
        print_acpi_message(buf);
        
        if (strncmp(table->signature, "FACP", 4) == 0) {
            parse_fadt((fadt_t*)table);
        }
    }
    
    if (!pm1a_cnt_blk) {
        print_acpi_message("[ACPI] Warning: FADT not found or PM1a_CNT_BLK is zero.");
    } else {
        print_acpi_message("[ACPI] Initialization complete.");
    }
}

void acpi_shutdown(void) {
    print_acpi_message("[ACPI] Initiating graceful shutdown sequence...");
    
    if (!pm1a_cnt_blk) {
        print_acpi_message("[ACPI] Error: PM1a_CNT_BLK not initialized. Cannot shutdown via ACPI.");
        print_acpi_message("[ACPI] Halting CPU...");
        while (1) {
            __asm__ __volatile__ ("cli; hlt");
        }
        return;
    }
    
    // SLP_EN is bit 13
    // SLP_TYP is bits 10-12
    uint16_t pm1a_val = (slp_typa << 10) | (1 << 13);
    outw(pm1a_cnt_blk, pm1a_val);
    
    if (pm1b_cnt_blk) {
        uint16_t pm1b_val = (slp_typb << 10) | (1 << 13);
        outw(pm1b_cnt_blk, pm1b_val);
    }
    
    // Fallback if QEMU/VirtualBox needs something else
    outw(0xB004, 0x2000);
    outw(0x604, 0x2000);
    outw(0x4004, 0x3400);
    
    print_acpi_message("[ACPI] Shutdown command sent. System should power off.");
    while (1) {
        __asm__ __volatile__ ("cli; hlt");
    }
}

void acpi_reboot(void) {
    print_acpi_message("[ACPI] Initiating system reboot...");
    
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
    
    while (1) {
        __asm__ __volatile__ ("cli; hlt");
    }
}

void acpi_sleep(int level) {
    if (!pm1a_cnt_blk) {
        print_acpi_message("[ACPI] Error: PM1a_CNT_BLK not initialized for sleep.");
        return;
    }
    
    char buf[128];
    sprintf(buf, "[ACPI] Entering Sleep State S%d...", level);
    print_acpi_message(buf);
    
    uint16_t slp_val = 0;
    if (level == 3) {
        slp_val = 5; // Usually 5 for S3 in some VMs, would need AML parse normally
    } else if (level == 4) {
        slp_val = 6;
    } else {
        slp_val = 1;
    }
    
    uint16_t pm1a_out = (slp_val << 10) | (1 << 13);
    outw(pm1a_cnt_blk, pm1a_out);
    
    if (pm1b_cnt_blk) {
        uint16_t pm1b_out = (slp_val << 10) | (1 << 13);
        outw(pm1b_cnt_blk, pm1b_out);
    }
    
    // Halt while sleeping
    __asm__ __volatile__ ("sti; hlt");
}

