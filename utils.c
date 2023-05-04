#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#include "utils.h"

int file_exists(const char *fn) { return access(fn, F_OK) == 0; }

inline unsigned int read32le(unsigned char *ptr)
{
	return ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
}

inline size_t readvint(unsigned char **stream)
{
	size_t result = 0, shift = 0;

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
