#include "format.h"
#include "filemap.h"

// clang-format off

extern const patch_format_t ips_format;
extern const patch_format_t ips32_format;
extern const patch_format_t bps_format;
extern const patch_format_t ups_format;

const patch_format_t *const patch_formats[] = { 
    &ips_format, 
    &ips32_format, 
    &ups_format, 
    &bps_format, 
    NULL 
};

// clang-format on