#ifndef NOTIFY_H
#define NOTIFY_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char title[64];
    char body[128];
    uint32_t accent_color;
    uint32_t timestamp_ticks;
    uint8_t active;
} notification_t;

void notify_init(void);
void notify_push(const char* title, const char* body, uint32_t accent);
void notify_render(void);
uint32_t notify_count(void);

#endif // NOTIFY_H
