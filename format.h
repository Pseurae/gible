#ifndef GIBLE_FORMAT_H
#define GIBLE_FORMAT_H

#include <stdint.h>
#include "mmap.h"

typedef struct patch_flags
{
} patch_flags_t;

typedef int (*patch_check)(uint8_t *);
typedef int (*patch_main)(char *, char *, char *, patch_flags_t *);

typedef struct patch_format
{
    char *name;
    patch_check check;
    patch_main main;
    const char **error_msgs;
} patch_format_t;

#define ERROR_PATCH_FILE_MMAP  (INT16_MAX - 1)
#define ERROR_INPUT_FILE_MMAP  (INT16_MAX - 2)
#define ERROR_OUTPUT_FILE_MMAP (INT16_MAX - 3)

#endif /* GIBLE_FORMAT_H */