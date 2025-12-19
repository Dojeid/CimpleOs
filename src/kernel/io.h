#ifndef IO_H
#define IO_H

#include <stdint.h>

// Declare these so everyone can use them
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

#endif