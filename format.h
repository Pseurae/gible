#ifndef FORMAT_H
#define FORMAT_H

#include "filemap.h"
#include "log.h"
#include <stdint.h>

// Common patch states
#define PATCH_RET_FAILURE        -1
#define PATCH_RET_SUCCESS        0
#define PATCH_RET_INVALID_PATCH  1
#define PATCH_RET_INVALID_INPUT  2
#define PATCH_RET_INVALID_OUTPUT 3

#define FLAG_CRC_PATCH           (1 << 0)
#define FLAG_CRC_INPUT           (1 << 1)
#define FLAG_CRC_OUTPUT          (1 << 2)
#define FLAG_CRC_ALL             (FLAG_CRC_PATCH | FLAG_CRC_INPUT | FLAG_CRC_OUTPUT)

#define PATCH_ERROR(...) (gible_error(__VA_ARGS__), PATCH_RET_FAILURE)

// By default, checksums are checked at the end.
typedef struct patch_flags
{
    // CRC byte:
    // xyz0 0000
    // x - patch, y - input, z - output
    unsigned char strict_crc; // Aborts patching on checksum mismatch
    unsigned char ignore_crc; // Don't even bother with checksum
} patch_flags_t;

typedef struct patch_context
{
    struct fn
    {
        char *patch;
        char *input;
        char *output;
    } fn;
    mmap_file_t patch;
    mmap_file_t input;
    mmap_file_t output;
    patch_flags_t *flags;
} patch_context_t;

typedef int (*patch_main)(patch_context_t *);

typedef struct patch_format
{
    char *name;
    char *header;
    unsigned char header_len;
    patch_main main;
} patch_format_t;

extern const patch_format_t ips_format;
extern const patch_format_t ips32_format;
extern const patch_format_t bps_format;
extern const patch_format_t ups_format;

#endif /* FORMAT_H */
