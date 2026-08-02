#include "gui/notify.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"

#define MAX_NOTIFICATIONS 5
#define NOTIFY_TIMEOUT_TICKS 400 // 4 seconds at 100Hz

static notification_t notify_ring[MAX_NOTIFICATIONS];
static uint32_t active_notifications = 0;

void notify_init(void) {
    memset(notify_ring, 0, sizeof(notify_ring));
    active_notifications = 0;
}

void notify_push(const char* title, const char* body, uint32_t accent) {
    if (!title || !body) return;
    extern volatile uint32_t timer_ticks;

    for (int i = MAX_NOTIFICATIONS - 1; i > 0; i--) {
        notify_ring[i] = notify_ring[i - 1];
    }

    strncpy(notify_ring[0].title, title, 63);
    notify_ring[0].title[63] = '\0';
    strncpy(notify_ring[0].body, body, 127);
    notify_ring[0].body[127] = '\0';
    notify_ring[0].accent_color = accent ? accent : 0x38BDF8;
    notify_ring[0].timestamp_ticks = timer_ticks;
    notify_ring[0].active = 1;
}

uint32_t notify_count(void) {
    extern volatile uint32_t timer_ticks;
    uint32_t count = 0;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (notify_ring[i].active && (timer_ticks - notify_ring[i].timestamp_ticks < NOTIFY_TIMEOUT_TICKS)) {
            count++;
        }
    }
    return count;
}

void notify_render(void) {
    extern volatile uint32_t timer_ticks;
    extern int screen_w, screen_h;
    int toast_w = 260;
    int toast_h = 56;
    int toast_x = screen_w - toast_w - 16;
    int base_y = screen_h - 45;

    int rendered_index = 0;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notify_ring[i].active) continue;
        if (timer_ticks - notify_ring[i].timestamp_ticks >= NOTIFY_TIMEOUT_TICKS) {
            notify_ring[i].active = 0;
            continue;
        }

        int toast_y = base_y - (rendered_index + 1) * (toast_h + 10);
        rendered_index++;

        // Shadow & Acrylic Background Card
        draw_rounded_rect_alpha(toast_x, toast_y, toast_w, toast_h, 8, 0x0F172A, 230);
        draw_rounded_rect_outline(toast_x, toast_y, toast_w, toast_h, 8, 1, notify_ring[i].accent_color);
        draw_rect(toast_x + 4, toast_y + 8, 4, toast_h - 16, notify_ring[i].accent_color);

        draw_string_shadow(toast_x + 16, toast_y + 8, 0xFFFFFF, 0x000000, notify_ring[i].title);
        draw_string(toast_x + 16, toast_y + 28, 0x94A3B8, notify_ring[i].body);
    }
}
