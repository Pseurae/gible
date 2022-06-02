#ifndef GIBLE_IPS_H
#define GIBLE_IPS_H

/*
IPS patches have 24 bit integers for storing offsets and
16 bit integers for storing size. This essentially limits
the size of the input and output file to 16MBs.
*/

#include "../format.h"

extern const patch_format_t ips_patch_format;
extern const patch_format_t ips32_patch_format;

#endif /* GIBLE_IPS_H */