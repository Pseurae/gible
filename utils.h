#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_COUNT(s) (sizeof(s) / sizeof(*s))

int file_exists(const char *fn);
size_t readvint(unsigned char **stream);
unsigned int read32le(unsigned char *ptr);

#endif /* UTIL_H */
