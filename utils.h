#ifndef GIBLE_UTIL_H
#define GIBLE_UTIL_H

#include <stdio.h>
#include <stdlib.h>

int file_exists(const char *fn);
size_t readvint(uint8_t **stream);
uint32_t read32le(uint8_t *ptr);

#endif /* GIBLE_UTIL_H */