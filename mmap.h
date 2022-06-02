#ifndef GIBLE_MMAP_H
#define GIBLE_MMAP_H

#include <stdint.h>
#include <stddef.h>

typedef enum gible_mmap_mode
{
    GIBLE_MMAP_READ,
    GIBLE_MMAP_WRITE,
    GIBLE_MMAP_READWRITE,
    GIBLE_MMAP_WRITEREAD,
    GIBLE_MMAP_MODE_COUNT
} gible_mmap_mode_t;

typedef struct gible_mmap_file
{
    char *fn;
    gible_mmap_mode_t mode;
    int status;
    size_t size;
    int fd;
    uint8_t *handle;
} gible_mmap_file_t;

gible_mmap_file_t gible_mmap_file_new(char *fn, gible_mmap_mode_t mode);
int gible_mmap_new(gible_mmap_file_t* f, size_t size);
int gible_mmap_open(gible_mmap_file_t *f);
void gible_mmap_close(gible_mmap_file_t *f);

#endif /* GIBLE_MMAP_H */