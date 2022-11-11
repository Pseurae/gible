#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
#include <windows.h>
#endif

typedef enum mmap_mode
{
    MMAP_READ,
    MMAP_WRITE,
    MMAP_READWRITE,
    MMAP_WRITEREAD,
    MMAP_MODE_COUNT
} mmap_mode_t;

typedef struct mmap_file
{
    char *fn;
    mmap_mode_t mode;
    int status;
    size_t size;
    uint8_t *handle;
#if defined(_WIN32)
    HANDLE filehandle;
    HANDLE maphandle;
#else
    int fd;
#endif
} mmap_file_t;

mmap_file_t mmap_file_new(char *fn, mmap_mode_t mode);
int mmap_create(mmap_file_t *f, size_t size);
int mmap_open(mmap_file_t *f);
void mmap_close(mmap_file_t *f);

#endif /* MMAP_H */
