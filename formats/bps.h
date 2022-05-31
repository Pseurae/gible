#ifndef GIBLE_BPS_H
#define GIBLE_BPS_H

#include <stdint.h>

int bps_check(uint8_t *patch);
int bps_patch_main(char *pfn, char *ifn, char *ofn);

enum
{
    BPS_SUCCESS = 0,
    BPS_INVALID_HEADER,
    BPS_TOO_SMALL,
    BPS_INVALID_ACTION,
    BPS_PATCH_CRC_NOMATCH,
    BPS_INPUT_CRC_NOMATCH,
    BPS_PATCH_FILE_MMAP,
    BPS_INPUT_FILE_MMAP,
    BPS_OUTPUT_FILE_MMAP,
};

#endif /* GIBLE_BPS_H */
