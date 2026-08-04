#ifndef AC97_H
#define AC97_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AC97_VENDOR_ID 0x8086
#define AC97_DEVICE_ID 0x2415

typedef struct {
    uint16_t nabm_bar;
    uint16_t nambar;
    int      pcm_out_active;
    uint8_t  master_volume;
} ac97_device_t;

int            ac97_init(void);
ac97_device_t* ac97_get_device(void);
int            ac97_play_pcm(uint32_t sample_rate, uint8_t channels, const uint8_t* pcm_data, uint32_t len);
void           ac97_set_volume(uint8_t vol_percent);

#ifdef __cplusplus
}
#endif

#endif // AC97_H
