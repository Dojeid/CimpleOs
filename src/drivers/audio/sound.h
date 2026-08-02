#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void sound_init(void);
void sound_play_pcm(uint32_t sample_rate, uint8_t channels, const uint8_t* pcm_data, uint32_t size);
void sound_set_volume(uint8_t volume_percent);
void sound_stop(void);

#endif // SOUND_H
