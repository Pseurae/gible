#ifndef HELPERS_FILEMAP_H
#define HELPERS_FILEMAP_H

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

typedef enum filemap_type
{
    FILEMAP_TYPE_CREATED,
    FILEMAP_TYPE_OPENED,
} filemap_type_t;

typedef struct filemap filemap_t;
typedef struct filemap_api
{
    int (*create)(filemap_t *);
    int (*open)(filemap_t *);
    void (*close)(filemap_t *);
} filemap_api_t;

typedef struct filemap
{
    filemap_status_t status;
    filemap_type_t type;
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
    const filemap_api_t *_api;
} filemap_t;

filemap_t filemap_new(const char *fn, int readonly, const filemap_api_t *const api);
int filemap_create(filemap_t *f, unsigned long size);
int filemap_open(filemap_t *f);
void filemap_close(filemap_t *f);

extern const filemap_api_t *const filemap_mmap_api;
extern const filemap_api_t *const filemap_buffer_api;

#endif /* HELPERS_FILEMAP_H */
