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

inline uint32_t read32le(uint8_t *ptr)
{
	return ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
}

inline size_t read_vint(uint8_t **stream)
{
	size_t result = 0, shift = 0;

	while (1)
	{
		uint8_t octet = *((*stream)++);
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
