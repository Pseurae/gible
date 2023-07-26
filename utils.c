#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <io.h>
#define F_OK   0
#define access _access
#else
#include <unistd.h>
#endif

#include "log.h"
#include "utils.h"
#include <string.h>

int file_exists(const char *fn)
{
    return access(fn, F_OK) == 0;
}

inline unsigned int read32le(const unsigned char *ptr)
{
    return ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
}

inline unsigned long readvint(unsigned char **stream)
{
    unsigned long result = 0, shift = 0;

    while (1)
    {
        unsigned char octet = *((*stream)++);
        if (octet & 0x80)
        {
            result += (octet & 0x7f) << shift;
            break;
        }
        result += (octet | 0x80) << shift;
        shift += 7;
    }
    return result;
}

int are_filenames_same(const char *pfn, const char *ifn, const char *ofn)
{
    if (!strcmp(pfn, ifn))
    {
        gible_error("Error: Patch and input filenames are the same.");
        return 1;
    }

    if (!strcmp(ifn, ofn))
    {
        gible_error("Error: Input and output filenames are the same.");
        return 1;
    }

    if (!strcmp(pfn, ofn))
    {
        gible_error("Error: Patch and output filenames are the same.");
        return 1;
    }

    return 0;
}
