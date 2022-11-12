/* Thin wrapper around mmap and MapViewOfFile. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "filemap.h"

static void mmap_file_init(mmap_file_t *f);
static int mmap_create_internal(mmap_file_t *f, size_t size);
static int mmap_open_internal(mmap_file_t *f);

mmap_file_t mmap_file_new(char *fn, mmap_mode_t mode)
{
    mmap_file_t f;
    mmap_file_init(&f);
    f.fn = fn;
    f.mode = mode;
    return f;
}

int mmap_create(mmap_file_t *f, size_t size)
{
    if (f->status) 
        return 0;

    if (f->mode == MMAP_READ)
        return 0;
    if (f->mode == MMAP_READWRITE)
        return 0;

    f->size = size;

    return mmap_create_internal(f, size);
}

int mmap_open(mmap_file_t *f)
{
    if (f->status) 
        return 0;

    return mmap_open_internal(f);
}

static void mmap_file_init(mmap_file_t *f)
{
    f->handle = NULL;
    f->status = 0;
}

#if defined(_WIN32)

#include <windows.h>

static const int mmap_mode_flags[MMAP_MODE_COUNT][4] = {
    [MMAP_READ] = { GENERIC_READ, OPEN_EXISTING, PAGE_READONLY, FILE_MAP_READ },
    [MMAP_WRITE] = { GENERIC_WRITE, CREATE_ALWAYS, PAGE_READWRITE, FILE_MAP_ALL_ACCESS },
    [MMAP_READWRITE] = { GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING, PAGE_READWRITE, FILE_MAP_ALL_ACCESS },
    [MMAP_WRITEREAD] = { GENERIC_READ | GENERIC_WRITE, CREATE_NEW, PAGE_READWRITE, FILE_MAP_ALL_ACCESS }
};

int mmap_create_internal(mmap_file_t *f, size_t size)
{
    const int *flags = mmap_mode_flags[f->mode];

    f->filehandle = CreateFileW(f->fn, flags[0], FILE_SHARE_READ, NULL, flags[1], FILE_ATTRIBUTE_NORMAL, NULL);

    if (f->filehandle == INVALID_HANDLE_VALUE) 
        return 0;

    f->maphandle = CreateFileMapping(f->filehandle, NULL, flags[2], 0, f->size, NULL);

    if (f->maphandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        return 0;
    }

    f->handle = (uint8_t *)MapViewOfFile(f->maphandle, flags[3], 0, 0, f->size);
    return 1;
};

int mmap_open_internal(mmap_file_t *f)
{
    const int *flags = mmap_mode_flags[f->mode];

    f->filehandle = CreateFileW(f->fn, flags[0], FILE_SHARE_READ, NULL, flags[1], FILE_ATTRIBUTE_NORMAL, NULL);

    if (f->filehandle == INVALID_HANDLE_VALUE) 
        return 0;

    f->size = GetFileSize(f->filehandle, NULL);

    f->maphandle = CreateFileMapping(f->filehandle, NULL, flags[2], 0, f->size, NULL);

    if (f->maphandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        return 0;
    }

    f->handle = (uint8_t *)MapViewOfFile(f->maphandle, flags[3], 0, 0, f->size);
    return 1;
};

int mmap_close(mmap_file_t *f)
{
    if (f->handle)
    {
        UnmapViewOfFile(f->handle);
        f->handle = NULL;
    }

    if (f->maphandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->maphandle);
        f->maphandle = INVALID_HANDLE_VALUE;
    }

    if (f->filehandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        f->filehandle = INVALID_HANDLE_VALUE;
    }
};

#else

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

static const int mmap_mode_flags[MMAP_MODE_COUNT][2] = {
    [MMAP_READ] = { O_RDONLY, PROT_READ },
    [MMAP_WRITE] = { O_RDWR | O_CREAT, PROT_WRITE },
    [MMAP_READWRITE] = { O_RDWR, PROT_READ | PROT_WRITE },
    [MMAP_WRITEREAD] = { O_RDWR | O_CREAT, PROT_READ | PROT_WRITE }
};

int mmap_create_internal(mmap_file_t *f, size_t size)
{
    const int *flags = mmap_mode_flags[f->mode];

    f->fd = open(f->fn, flags[0], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd < 0)
        return 0;

    ftruncate(f->fd, size);
    f->handle = (uint8_t *)mmap(0, f->size, flags[1], MAP_SHARED, f->fd, 0);

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

int mmap_open_internal(mmap_file_t *f)
{
    if (f->status)
        return 0;

    const int *flags = mmap_mode_flags[f->mode];

    f->fd = open(f->fn, flags[0], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd < 0)
        return 0;

    struct stat p_stat;
    fstat(f->fd, &p_stat);
    f->size = p_stat.st_size;

    f->handle = (uint8_t *)mmap(0, f->size, flags[1], MAP_SHARED, f->fd, 0);

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

void mmap_close(mmap_file_t *f)
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
