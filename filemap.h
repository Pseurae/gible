#ifndef MMAP_H
#define MMAP_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#endif

typedef struct mmap_file
{
    char *fn;
    int readonly;
    int32_t status;
    size_t size;
    unsigned char *handle;
#if defined(_WIN32)
    HANDLE filehandle;
    HANDLE maphandle;
#else
    int fd;
#endif
} mmap_file_t;

mmap_file_t mmap_file_new(char *fn, int readonly);
int mmap_create(mmap_file_t *f, size_t size);
int mmap_open(mmap_file_t *f);
void mmap_close(mmap_file_t *f);

#endif /* MMAP_H */
