#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ARRAY_COUNT(s) (sizeof(s) / sizeof(*s))

int file_exists(const char *fn);
size_t readvint(uint8_t **stream);
uint32_t read32le(uint8_t *ptr);

#endif /* UTIL_H */
