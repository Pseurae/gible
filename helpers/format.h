#ifndef HELPERS_FORMAT_H
#define HELPERS_FORMAT_H

#include "helpers/filemap.h"
#include "helpers/log.h"
#include <stdint.h>

// Common return values
#define APPLY_RET_FAILURE          -1
#define APPLY_RET_SUCCESS          0
#define APPLY_RET_INVALID_PATCH    1
#define APPLY_RET_INVALID_INPUT    2
#define APPLY_RET_INVALID_OUTPUT   3

#define CREATE_RET_FAILURE         -1
#define CREATE_RET_SUCCESS         0
#define CREATE_RET_INVALID_PATCHED 1
#define CREATE_RET_INVALID_BASE    2
#define CREATE_RET_INVALID_OUTPUT  3

// CRC flags
#define CRC_PATCH         0
#define CRC_INPUT         1
#define CRC_OUTPUT        2

#define FLAG_CRC_PATCH    (1 << CRC_PATCH)
#define FLAG_CRC_INPUT    (1 << CRC_INPUT)
#define FLAG_CRC_OUTPUT   (1 << CRC_OUTPUT)
#define FLAG_CRC_ALL      (FLAG_CRC_PATCH | FLAG_CRC_INPUT | FLAG_CRC_OUTPUT)

#define APPLY_ERROR(...)  (gible_error(__VA_ARGS__), APPLY_RET_FAILURE)
#define CREATE_ERROR(...) (gible_error(__VA_ARGS__), CREATE_RET_FAILURE)

// By default, checksums are checked at the end.
typedef struct patch_flags
{
    // CRC byte:
    // xyz0 0000
    // x - patch, y - input, z - output
    unsigned char strict_crc; // Aborts patching on checksum mismatch
    unsigned char ignore_crc; // Don't even bother with checksum
} patch_flags_t;

typedef struct patch_apply_context
{
    struct
    {
        const char *patch;
        const char *input;
        const char *output;
    } fn;
    filemap_t patch;
    filemap_t input;
    filemap_t output;
    patch_flags_t *flags;
} patch_apply_context_t;

typedef struct patch_create_context
{
    const char *patchtype;
    struct
    {
        const char *patched;
        const char *base;
        const char *output;
    } fn;
    filemap_t patched;
    filemap_t base;
    filemap_t output;
} patch_create_context_t;

typedef int (*apply_main)(patch_apply_context_t *);
typedef int (*create_main)(patch_create_context_t *);

typedef int (*apply_check)(patch_apply_context_t *);
typedef int (*create_check)(patch_create_context_t *);

typedef struct patch_format
{
    const char *name;
    const char *header;
    const char *ext;

    apply_main apply_main;
    create_main create_main;

    apply_check apply_check;
    create_check create_check;
} patch_format_t;

extern const patch_format_t *const patch_formats[];

#endif /* HELPERS_FORMAT_H */
