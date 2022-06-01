#ifndef GIBLE_IPS_H
#define GIBLE_IPS_H

/*
IPS patches have 24 bit integers for storing offsets and
16 bit integers for storing size. This essentially limits
the size of the input and output file to 16MBs.
*/

#include <stdint.h>

int ips_check(uint8_t *patch);
int ips_patch_main(char *pfn, char *ifn, char *ofn);

enum
{
    IPS_SUCCESS = 0,
    IPS_INVALID_HEADER,
    IPS_TOO_SMALL,
    IPS_PATCH_FILE_MMAP,
    IPS_INPUT_FILE_MMAP,
    IPS_OUTPUT_FILE_MMAP,
    IPS_ERROR_COUNT
};

#endif /* GIBLE_IPS_H */