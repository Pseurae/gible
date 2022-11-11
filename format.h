#ifndef FORMAT_H
#define FORMAT_H

#include <stdint.h>
#include "filemap.h"

#define ERROR_PATCH_FILE_MMAP (INT16_MAX - 1)
#define ERROR_INPUT_FILE_MMAP (INT16_MAX - 2)
#define ERROR_OUTPUT_FILE_MMAP (INT16_MAX - 3)

#define FLAG_CRC_PATCH (1 << 7)
#define FLAG_CRC_INPUT (1 << 6)
#define FLAG_CRC_OUTPUT (1 << 5)
#define FLAG_CRC_ALL (FLAG_CRC_PATCH | FLAG_CRC_INPUT | FLAG_CRC_OUTPUT)

// By default, checksums are checked at the end.
typedef struct patch_flags
{
    // CRC byte:
    // xyz0 0000
    // x - patch, y - input, z - output
    uint8_t strict_crc;     // Aborts patching on checksum mismatch
    uint8_t ignore_crc;     // Don't even bother with checksum
    uint8_t use_filebuffer; // Not implemented. Yet.
    uint8_t verbose;        // Is the output verbose?
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

extern const patch_format_t ips_patch_format;
extern const patch_format_t ips32_patch_format;
extern const patch_format_t ups_patch_format;
extern const patch_format_t bps_patch_format;

#endif /* FORMAT_H */
