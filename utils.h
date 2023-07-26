#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_COUNT(s) (sizeof(s) / sizeof(*s))

int file_exists(const char *fn);
unsigned long readvint(unsigned char **stream);
unsigned int read32le(const unsigned char *ptr);
int are_filenames_same(const char *pfn, const char *ifn, const char *ofn);

#endif /* UTIL_H */
