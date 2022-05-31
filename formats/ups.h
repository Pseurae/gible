#ifndef GIBLE_UPS_H
#define GIBLE_UPS_H

/*
IPS patches have 24 bit integers for storing offsets and
16 bit integers for storing size. This essentially limits
the size of the input and output file to 16MBs.
*/

#include <stdint.h>

int ups_check(uint8_t *patch);
int ups_patch_main(char *pfn, char *ifn, char *ofn);

enum
{
    UPS_SUCCESS = 0,
    UPS_INVALID_HEADER,
    UPS_TOO_SMALL,
    UPS_INPUT_CRC_NOMATCH,
    UPS_PATCH_CRC_NOMATCH,
    UPS_OUTPUT_CRC_NOMATCH,
    UPS_PATCH_FILE_MMAP,
    UPS_INPUT_FILE_MMAP,
    UPS_OUTPUT_FILE_MMAP,
};

#endif /* GIBLE_UPS_H */