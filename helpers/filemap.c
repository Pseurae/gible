/* Thin wrapper around mmap and MapViewOfFile. */

#include "helpers/filemap.h"
#include <stdio.h> // fopen, fclose, fseek, ftell
#include <stdlib.h> // malloc, free

// -------------------------------------------------
// Filemap API
// -------------------------------------------------

static void filemap_init(filemap_t *f);

filemap_t filemap_new(const char *fn, int readonly, const filemap_api_t *const api)
{
    filemap_t f;
    filemap_init(&f);
    f.fn = fn;
    f.readonly = readonly;
    f._api = api;
    return f;
}

static void filemap_init(filemap_t *f)
{
    f->handle = NULL;
    f->status = FILEMAP_NOT_OPENED;
    f->size = 0;
}

int filemap_create(filemap_t *f, unsigned long size)
{
    if (f->status != FILEMAP_NOT_OPENED)
        return 0;

    if (f->readonly)
        return 0;

    f->type = FILEMAP_TYPE_CREATED;
    f->size = size;

    return f->_api->create(f);
}

int filemap_open(filemap_t *f)
{
    if (f->status != FILEMAP_NOT_OPENED)
        return 0;

    f->type = FILEMAP_TYPE_OPENED;
    return f->_api->open(f);
}

void filemap_close(filemap_t *f)
{
    f->_api->close(f);
}

// -------------------------------------------------
// Memory Mapped File Implementation
// -------------------------------------------------

#if defined(_WIN32)

#include <windows.h>

static const int share_flag = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

static int filemap_mmap_create(filemap_t *f)
{
    int file_access = GENERIC_READ | GENERIC_WRITE;
    int creation_disposition = CREATE_ALWAYS;
    int mapping_attr = PAGE_READWRITE;
    int map_access = FILE_MAP_ALL_ACCESS;

    f->filehandle = CreateFile(f->fn, file_access, share_flag, NULL, creation_disposition, 0, NULL);

    if (f->filehandle == INVALID_HANDLE_VALUE)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->maphandle = CreateFileMappingW(f->filehandle, NULL, mapping_attr, 0, f->size, NULL);

    if (f->maphandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->handle = (unsigned char *)MapViewOfFile(f->maphandle, map_access, 0, 0, 0);
    f->status = FILEMAP_OK;
    return 1;
};

static int filemap_mmap_open(filemap_t *f)
{
    int file_access = GENERIC_READ | (!f->readonly ? GENERIC_WRITE : 0);
    int creation_disposition = OPEN_EXISTING;
    int mapping_attr = f->readonly ? PAGE_READONLY : PAGE_READWRITE;
    int map_access = f->readonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

    f->filehandle = CreateFile(f->fn, file_access, share_flag, NULL, creation_disposition, 0, NULL);

    if (f->filehandle == INVALID_HANDLE_VALUE)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    LARGE_INTEGER i;
    if (GetFileSizeEx(f->filehandle, &i))
    {
        f->size = (unsigned long)i.QuadPart;
    }
    else
    {
        CloseHandle(f->filehandle);
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->maphandle = CreateFileMappingW(f->filehandle, NULL, mapping_attr, 0, f->size, NULL);

    if (f->maphandle == INVALID_HANDLE_VALUE)
    {
        CloseHandle(f->filehandle);
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->handle = (unsigned char *)MapViewOfFile(f->maphandle, map_access, 0, 0, 0);
    f->status = FILEMAP_OK;
    return 1;
};

static void filemap_mmap_close(filemap_t *f)
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

    f->status = FILEMAP_NOT_OPENED;
};

#else

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int filemap_mmap_create(filemap_t *f)
{
    int open_flags = O_RDWR | O_CREAT | O_TRUNC;
    int protect_flags = PROT_READ | PROT_WRITE;

    // creat doesn't work here.
    f->fd = open(f->fn, open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd == -1)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    if (ftruncate(f->fd, f->size) == -1)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->handle = (unsigned char *)mmap(0, f->size, protect_flags, MAP_SHARED, f->fd, 0);

    if (f->handle == MAP_FAILED)
    {
        f->handle = 0;
        close(f->fd);
        f->fd = -1;
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->status = FILEMAP_OK;
    return 1;
}

static int filemap_mmap_open(filemap_t *f)
{
    int open_flags = f->readonly ? O_RDONLY : O_RDWR;
    int protect_flags = PROT_READ | (f->readonly ? 0 : PROT_WRITE);

    f->fd = open(f->fn, open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (f->fd == -1)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    struct stat p_stat;
    fstat(f->fd, &p_stat);
    f->size = p_stat.st_size;

    f->handle = (unsigned char *)mmap(0, f->size, protect_flags, MAP_SHARED, f->fd, 0);

    if ((void *)f->handle == MAP_FAILED)
    {
        f->handle = 0;
        close(f->fd);
        f->fd = -1;
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->status = FILEMAP_OK;
    return 1;
}

static void filemap_mmap_close(filemap_t *f)
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

    f->status = FILEMAP_NOT_OPENED;
}
#endif

// -------------------------------------------------
// Allocated Buffer File Implementation
// -------------------------------------------------


static int filemap_buffer_create(filemap_t *f)
{
    if (!(f->handle = malloc(sizeof(char) * f->size)))
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->status = FILEMAP_OK;
    return 1;
}

static int filemap_buffer_open(filemap_t *f)
{
    FILE *fp = fopen(f->fn, "r");

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    f->size = ftell(fp);
    if (!filemap_buffer_create(f))
        return 0;

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    if (fread(f->handle, sizeof(char), f->size, fp) != f->size)
    {
        f->status = FILEMAP_ERROR;
        return 0;
    }

    fclose(fp);

    f->status = FILEMAP_OK;
    return 1;
}

static void filemap_buffer_close(filemap_t *f)
{
    if (f->handle)
    {
        if (f->type == FILEMAP_TYPE_CREATED || (f->type == FILEMAP_TYPE_OPENED && !f->readonly))
        {
            FILE *fp = fopen(f->fn, "w");
            fwrite(f->handle, sizeof(char), f->size, fp);
            fclose(fp);
        }

        free(f->handle);
    }
}

// -------------------------------------------------
// API Definitions
// -------------------------------------------------

const filemap_api_t filemap_mmap_api__ = {
    .create = filemap_mmap_create,
    .open = filemap_mmap_open,
    .close = filemap_mmap_close
};

const filemap_api_t filemap_buffer_api__ = {
    .create = filemap_buffer_create,
    .open = filemap_buffer_open,
    .close = filemap_buffer_close
};

const filemap_api_t *const filemap_mmap_api = &filemap_mmap_api__;
const filemap_api_t *const filemap_buffer_api = &filemap_buffer_api__;
