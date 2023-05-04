/* Thin wrapper around mmap and MapViewOfFile. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "filemap.h"

static void mmap_file_init(mmap_file_t *f);
static int mmap_create_internal(mmap_file_t *f);
static int mmap_open_internal(mmap_file_t *f);

mmap_file_t mmap_file_new(char *fn, int readonly)
{
    mmap_file_t f;
    mmap_file_init(&f);
    f.fn = fn;
    f.readonly = readonly;
    return f;
}

int mmap_create(mmap_file_t *f, size_t size)
{
    if (f->status) 
        return 0;

    if (f->readonly)
        return 0;

    f->size = size;

    return mmap_create_internal(f);
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

static const int share_flag = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

int mmap_create_internal(mmap_file_t *f)
{
    int file_access = GENERIC_READ | GENERIC_WRITE;
    int creation_disposition = CREATE_ALWAYS;
    int mapping_attr = PAGE_READWRITE;
    int map_access = FILE_MAP_ALL_ACCESS;

    f->filehandle = CreateFile(
        f->fn, file_access, share_flag, 
        NULL, creation_disposition, 0, NULL
    );

    if (f->filehandle == INVALID_HANDLE_VALUE) 
    {
        f->status = -1;
        return 0;
    }

    f->maphandle = CreateFileMappingW(
        f->filehandle, NULL, mapping_attr, 
        0, f->size, NULL
    );

    if (f->maphandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        f->status = -1;
        return 0;
    }

    f->handle = (unsigned char *)MapViewOfFile(f->maphandle, map_access, 0, 0, 0);
    f->status = 1;
    return 1;
};

int mmap_open_internal(mmap_file_t *f)
{
    int file_access = GENERIC_READ | (!f->readonly ? GENERIC_WRITE : 0);
    int creation_disposition = OPEN_EXISTING;
    int mapping_attr = f->readonly ? PAGE_READONLY : PAGE_READWRITE;
    int map_access = f->readonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

    f->filehandle = CreateFile(
        f->fn, file_access, share_flag, NULL, 
        creation_disposition, 0, NULL
    );

    if (f->filehandle == INVALID_HANDLE_VALUE) 
    {
        f->status = -1;
        return 0;
    }

    LARGE_INTEGER i;
    if (GetFileSizeEx(f->filehandle, &i)) {
        f->size = (size_t)i.QuadPart;
    } else {
        CloseHandle(f->filehandle);
        return 0;
    }

    f->maphandle = CreateFileMappingW(
        f->filehandle, NULL, mapping_attr, 
        0, f->size, NULL
    );

    if (f->maphandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        f->status = -1;
        return 0;
    }

    f->handle = (unsigned char *)MapViewOfFile(f->maphandle, map_access, 0, 0, 0);
    f->status = 1;
    return 1;
};

void mmap_close(mmap_file_t *f)
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

int mmap_create_internal(mmap_file_t *f)
{
    int open_flags = O_RDWR | O_CREAT | O_TRUNC;
    int protect_flags = PROT_READ | PROT_WRITE;

    // creat doesn't work here.
    f->fd = open(f->fn, open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd < 0)
    {
        f->status = -1;
        return 0;
    }

    ftruncate(f->fd, f->size);
    f->handle = (unsigned char *)mmap(0, f->size, protect_flags, MAP_SHARED, f->fd, 0);

    if (f->handle == MAP_FAILED)
    {
        f->handle = 0;
        close(f->fd);
        f->fd = -1;
        f->status = -1;
        return 0;
    }

    f->status = 1;
    return 1;
}

int mmap_open_internal(mmap_file_t *f)
{
    int open_flags = f->readonly ? O_RDONLY : O_RDWR;
    int protect_flags = PROT_READ | (f->readonly ? 0 : PROT_WRITE);

    f->fd = open(f->fn, open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd < 0)
    {
        f->status = -1;
        return 0;
    }

    struct stat p_stat;
    fstat(f->fd, &p_stat);
    f->size = p_stat.st_size;

    f->handle = (unsigned char *)mmap(0, f->size, protect_flags, MAP_SHARED, f->fd, 0);

    if (f->handle == MAP_FAILED)
    {
        f->handle = 0;
        close(f->fd);
        f->fd = -1;
        f->status = -1;
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
