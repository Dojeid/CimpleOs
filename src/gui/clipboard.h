#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <stddef.h>
#include <stdint.h>

#define CLIPBOARD_MAX_SIZE 4096

void clipboard_init(void);
void clipboard_set(const char* text, size_t len);
const char* clipboard_get(void);
size_t clipboard_get_length(void);

#endif // CLIPBOARD_H
