#include "drivers/audio/ac97.h"
#include "drivers/bus/pci.h"
#include "lib/io.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

static ac97_device_t g_ac97_dev = {0};

int ac97_init(void) {
    struct pci_device dev;
    if (!pci_find_by_id(AC97_VENDOR_ID, AC97_DEVICE_ID, &dev)) {
        vga_print("[AC97] Intel AC97 Audio Controller (Vendor 0x8086 Device 0x2415) not found.\n");
        return -1;
    }

    uint32_t bar0 = dev.bar0 ? dev.bar0 : pci_read_config(dev.bus, dev.slot, dev.func, 0x10);
    uint32_t bar1 = dev.bar1 ? dev.bar1 : pci_read_config(dev.bus, dev.slot, dev.func, 0x14);

    g_ac97_dev.nambar   = (uint16_t)(bar0 & ~0x3);
    g_ac97_dev.nabm_bar = (uint16_t)(bar1 & ~0x3);

    // Enable Bus Master & I/O Space Access
    uint32_t pci_cmd = pci_read_config(dev.bus, dev.slot, dev.func, 0x04);
    pci_cmd |= 0x05;
    pci_write_config(dev.bus, dev.slot, dev.func, 0x04, pci_cmd);

    // Set Master Volume & PCM Out Volume
    outw(g_ac97_dev.nambar + 0x02, 0x0808);
    outw(g_ac97_dev.nambar + 0x18, 0x0808);

    g_ac97_dev.pcm_out_active = 1;
    g_ac97_dev.master_volume = 80;

    char log[128];
    snprintf(log, sizeof(log), "[AC97] Intel AC97 Audio Controller active (NAMBAR: 0x%04X, NABMBAR: 0x%04X).\n",
             g_ac97_dev.nambar, g_ac97_dev.nabm_bar);
    vga_print(log);
    return 0;
}

ac97_device_t* ac97_get_device(void) {
    return &g_ac97_dev;
}

int ac97_play_pcm(uint32_t sample_rate, uint8_t channels, const uint8_t* pcm_data, uint32_t len) {
    if (!g_ac97_dev.nambar || !pcm_data || len == 0) return -1;
    extern int sound_play_pcm(uint32_t sample_rate, uint8_t channels, const uint8_t* data, uint32_t length);
    return sound_play_pcm(sample_rate, channels, pcm_data, len);
}

void ac97_set_volume(uint8_t vol_percent) {
    if (vol_percent > 100) vol_percent = 100;
    g_ac97_dev.master_volume = vol_percent;
    if (g_ac97_dev.nambar) {
        uint16_t attenuation = (uint16_t)((100 - vol_percent) * 31 / 100);
        uint16_t val = (attenuation << 8) | attenuation;
        outw(g_ac97_dev.nambar + 0x02, val);
        outw(g_ac97_dev.nambar + 0x18, val);
    }
}
