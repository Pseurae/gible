#ifndef FILEMAP_H
#define FILEMAP_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#endif

typedef enum filemap_status
{
    FILEMAP_NOT_OPENED = -1,
    FILEMAP_ERROR = 0,
    FILEMAP_OK = 1
} filemap_status_t;

typedef struct filemap
{
    filemap_status_t status;
    const char *fn;
    unsigned char readonly;
    unsigned long size;
    unsigned char *handle;
#if defined(_WIN32)
    HANDLE filehandle;
    HANDLE maphandle;
#else
    int fd;
#endif
} filemap_t;

filemap_t filemap_new(const char *fn, int readonly);
int filemap_create(filemap_t *f, unsigned long size);
int filemap_open(filemap_t *f);
void filemap_close(filemap_t *f);

#endif /* FILEMAP_H */
