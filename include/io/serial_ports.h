#pragma once
#include <stdint.h>

#define SERIAL_COM1_BASE 0x3F8

void serial_write(unsigned short com, char *log);
void serial_write_hex(uint32_t num);