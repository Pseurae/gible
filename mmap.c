/* Thin wrapper around mmap and MapViewToFile.
 * MapViewToFile still needs to be done.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "mmap.h"

static void gible_mmap_file_init(gible_mmap_file_t *f)
{
    f->handle = NULL;
    f->status = 0;
}

gible_mmap_file_t gible_mmap_file_new(char *fn, gible_mmap_mode_t mode)
{
    gible_mmap_file_t f;
    gible_mmap_file_init(&f);
    f.fn = fn;
    f.mode = mode;
    return f;
}

#if defined(_WIN32)
#else

static const int gible_open_flags[GIBLE_MMAP_MODE_COUNT] = {
    [GIBLE_MMAP_READ] = O_RDONLY,
    [GIBLE_MMAP_WRITE] = O_RDWR | O_CREAT,
    [GIBLE_MMAP_READWRITE] = O_RDWR,
    [GIBLE_MMAP_WRITEREAD] = O_RDWR | O_CREAT,
};

static const int gible_mmap_flags[GIBLE_MMAP_MODE_COUNT] = {
    [GIBLE_MMAP_READ] = PROT_READ,
    [GIBLE_MMAP_WRITE] = PROT_WRITE,
    [GIBLE_MMAP_READWRITE] = PROT_READ | PROT_WRITE,
    [GIBLE_MMAP_WRITEREAD] = PROT_READ | PROT_WRITE,
};

inline int gible_mmap_new(gible_mmap_file_t *f, size_t size)
{
#define error() (f->status = 0, 0)
    FILE *temp_out = fopen(f->fn, "w");
    fseek(temp_out, size - 1, SEEK_SET);
    fputc(0, temp_out);
    fclose(temp_out);
    return gible_mmap_open(f);
#undef error
}

int gible_mmap_open(gible_mmap_file_t *f)
{
    if (f->status) return 0;

    int open_flags = gible_open_flags[f->mode];
    int mmap_flags = gible_mmap_flags[f->mode];

    f->fd = open(f->fn, open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd < 0)
        return 0;

    struct stat p_stat;
    fstat(f->fd, &p_stat);
    f->size = p_stat.st_size;

    f->handle = (uint8_t *)mmap(0, f->size, mmap_flags, MAP_SHARED, f->fd, 0);

    if (f->handle == MAP_FAILED)
    {
        f->handle = 0;
        close(f->fd);
        f->fd = -1;
        return 0;
    }

    f->status = 1;
    return 1;
}

void gible_mmap_close(gible_mmap_file_t *f)
{
    if (f->handle)
    {
        munmap(f->handle, f->size);
        f->handle = NULL;
    }

    if (f->fd >= 0)
    {
        close(f->fd);
        f->fd = -1;
    }
}
#endif