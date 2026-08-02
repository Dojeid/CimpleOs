#include "gui/clipboard.h"
#include "lib/string.h"

static char clipboard_buffer[CLIPBOARD_MAX_SIZE];
static size_t clipboard_length = 0;

void clipboard_init(void) {
    memset(clipboard_buffer, 0, sizeof(clipboard_buffer));
    clipboard_length = 0;
}

void clipboard_set(const char* text, size_t len) {
    if (!text) return;
    if (len >= CLIPBOARD_MAX_SIZE) len = CLIPBOARD_MAX_SIZE - 1;
    memcpy(clipboard_buffer, text, len);
    clipboard_buffer[len] = '\0';
    clipboard_length = len;
}

const char* clipboard_get(void) {
    return clipboard_buffer;
}

size_t clipboard_get_length(void) {
    return clipboard_length;
}
