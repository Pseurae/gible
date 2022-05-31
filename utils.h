#ifndef GIBLE_UTIL_H
#define GIBLE_UTIL_H

#include <stdio.h>
#include <stdlib.h>

int file_exists(const char *fn);
size_t read_vint(uint8_t **stream);
uint32_t read32le(uint8_t *ptr);

void crc32_adjust(uint32_t *crc32, uint8_t input);
uint32_t crc32_calculate(const uint8_t *data, unsigned length);
void crc32_populate(const uint8_t *data, unsigned length, uint32_t *crc);

#endif /* GIBLE_UTIL_H */