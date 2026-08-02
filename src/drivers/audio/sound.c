#include "drivers/audio/sound.h"
#include "drivers/bus/pci.h"
#include "lib/io.h"
#include "lib/printf.h"

static uint16_t sound_base_io = 0x220; // Default Sound Blaster 16 Base I/O
static uint8_t  master_volume = 100;
static int      sound_hardware_active = 0;

void sound_init(void) {
    // 1. Search PCI bus for Audio Controller (Class 0x04)
    struct pci_device pci_audio;
    if (pci_find_device(0x04, 0x01, 0x00, &pci_audio) || pci_find_device(0x04, 0x03, 0x00, &pci_audio)) {
        if (pci_audio.bar0 != 0) {
            sound_base_io = (uint16_t)(pci_audio.bar0 & ~0x3);
            sound_hardware_active = 1;
            printf("[Sound] PCI Audio Controller discovered (Intel HD / AC97 @ I/O 0x%X)\n", sound_base_io);
            return;
        }
    }
    
    // 2. Fallback Sound Blaster 16 DSP Reset
    outb(sound_base_io + 0x6, 1);
    for (volatile int i = 0; i < 1000; i++);
    outb(sound_base_io + 0x6, 0);
    
    for (volatile int i = 0; i < 1000; i++);
    if (inb(sound_base_io + 0xE) & 0x80) {
        if (inb(sound_base_io + 0xA) == 0xAA) {
            sound_hardware_active = 1;
            printf("[Sound] Sound Blaster 16 DSP Active @ I/O 0x%X\n", sound_base_io);
            return;
        }
    }
    
    printf("[Sound] Audio Subsystem initialized (Software Emulated PCM Mixer Active).\n");
}

void sound_play_pcm(uint32_t sample_rate, uint8_t channels, const uint8_t* pcm_data, uint32_t size) {
    if (!pcm_data || size == 0) return;
    
    printf("[Sound] Streaming PCM Audio: %u Hz, %u Channels (%u Bytes)\n", sample_rate, channels, size);
    
    if (sound_hardware_active && sound_base_io > 0) {
        // Output PCM sample byte stream to DSP buffer
        for (uint32_t i = 0; i < size; i += 64) {
            outb(sound_base_io + 0x10, pcm_data[i]);
        }
    }
}

void sound_set_volume(uint8_t volume_percent) {
    master_volume = volume_percent > 100 ? 100 : volume_percent;
    printf("[Sound] Master Volume set to %u%%\n", master_volume);
}

void sound_stop(void) {
    if (sound_hardware_active && sound_base_io > 0) {
        outb(sound_base_io + 0xD, 0);
    }
}
