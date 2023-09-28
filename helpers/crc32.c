#include "helpers/crc32.h"
#include <stdint.h>
#include <stdlib.h>

// clang-format off

static const unsigned int crc_halfbyte_lookup16[16] =
{
    0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
    0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C,
};

// clang-format on

unsigned int crc32(const void *data, unsigned long length, unsigned int prev)
{
    unsigned int crc = ~prev;
    const unsigned char *current = (const unsigned char *)data;

    while (length-- != 0)
    {
        crc = crc_halfbyte_lookup16[(crc ^ *current) & 0x0F] ^ (crc >> 4);
        crc = crc_halfbyte_lookup16[(crc ^ (*current >> 4)) & 0x0F] ^ (crc >> 4);
        current++;
    }

    return ~crc;
}
