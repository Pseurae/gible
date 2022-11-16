#include <stdio.h>
#include <stdint.h>

#include "../format.h"
#include "../filemap.h"

enum ips_error
{
    IPS_SUCCESS = 0,
    IPS_INVALID_HEADER,
    IPS_TOO_SMALL,
    IPS_NO_FOOTER,
    IPS_ERROR_COUNT
};

static const char *ips_error_messages[IPS_ERROR_COUNT];
static inline int ips_check(uint8_t *patch);
static inline int ips32_check(uint8_t *patch);
static int ips_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags);
static int ips32_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags);

const patch_format_t ips_patch_format = { "IPS", ips_check, ips_patch, ips_error_messages };
const patch_format_t ips32_patch_format = { "IPS32", ips32_check, ips32_patch, ips_error_messages };

static const char *ips_error_messages[IPS_ERROR_COUNT] = {
    [IPS_SUCCESS] = "IPS patching successful.",
    [IPS_INVALID_HEADER] = "Invalid header for an IPS file.",
    [IPS_TOO_SMALL] = "Patch file is too small to be an IPS file.",
    [IPS_NO_FOOTER] = "EOF footer not found."
};

static inline int ips_check(uint8_t *patch)
{
    char patch_header[5] = {'P', 'A', 'T', 'C', 'H'};

    for (int i = 0; i < 5; i++)
    {
        if (patch_header[i] != patch[i])
            return 0;
    }

    return 1;
}

static inline int ips32_check(uint8_t *patch)
{
    char patch_header[5] = {'I', 'P', 'S', '3', '2'};

    for (int i = 0; i < 5; i++)
    {
        if (patch_header[i] != patch[i])
            return 0;
    }

    return 1;
}

static int ips_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags)
{
#define error(i)  \
    do            \
    {             \
        ret = i;  \
        goto end; \
    } while (0)

    (void)flags;
    int ret = IPS_SUCCESS;

    uint8_t *patch, *patchend, *input, *output;
    mmap_file_t patchmf, inputmf, outputmf;

    patchmf = mmap_file_new(pfn, MMAP_READ);
    mmap_open(&patchmf);

    if (patchmf.status == -1)
        error(ERROR_PATCH_FILE_MMAP);

    if (patchmf.size < 8)
        error(IPS_TOO_SMALL);

    patch = patchmf.handle;
    patchend = patch + patchmf.size;

#define patch8() ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch24() ((patch + 3 < patchend) ? (patch += 3, (patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    if (patch8() != 'P' || patch8() != 'A' || patch8() != 'T' || patch8() != 'C' || patch8() != 'H')
        error(IPS_INVALID_HEADER); // Never gonna get called, unless the function gets used directly.

    inputmf = mmap_file_new(ifn, MMAP_READ);
    mmap_open(&inputmf);

    if (inputmf.status == -1)
        error(ERROR_INPUT_FILE_MMAP);

    input = inputmf.handle;

    FILE *temp_out = fopen(ofn, "w+");
    fwrite(input, sizeof(char), inputmf.size, temp_out);
    fclose(temp_out);

    mmap_close(&inputmf);

    outputmf = mmap_file_new(ofn, MMAP_READWRITE);
    mmap_open(&outputmf);

    if (!outputmf.status)
        error(ERROR_OUTPUT_FILE_MMAP);

    output = outputmf.handle;

    while (patch < patchend - 3)
    {
        uint32_t offset = patch24();
        uint16_t size = patch16();

        uint8_t *outputoff = (output + offset);

        if (size)
        {
            while (size--)
                *(outputoff++) = patch8();
        }
        else
        {
            size = patch16();
            uint8_t byte = patch8();

            while (size--)
                *(outputoff++) = byte;
        }
    }

    if (patch8() != 'E' || patch8() != 'O' || patch8() != 'F')
        error(IPS_NO_FOOTER);

#undef error
#undef patch8
#undef patch16
#undef patch24

end:
    mmap_close(&patchmf);
    mmap_close(&inputmf);
    mmap_close(&outputmf);

    return ret;
}

static int ips32_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags)
{
#define error(i)  \
    do            \
    {             \
        ret = i;  \
        goto end; \
    } while (0)

    (void)flags;
    int ret = IPS_SUCCESS;

    uint8_t *patch, *patchend, *input, *output;
    mmap_file_t patchmf, inputmf, outputmf;

    patchmf = mmap_file_new(pfn, MMAP_READ);
    mmap_open(&patchmf);

    if (patchmf.status == -1)
        error(ERROR_PATCH_FILE_MMAP);

    if (patchmf.size < 9)
        error(IPS_TOO_SMALL);

    patch = patchmf.handle;
    patchend = patch + patchmf.size;

#define patch8() ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch32() ((patch + 4 < patchend) ? (patch += 4, (patch[-3] << 24 | patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    if (patch8() != 'I' || patch8() != 'P' || patch8() != 'S' || patch8() != '3' || patch8() != '2')
        error(IPS_INVALID_HEADER); // Never gonna get called, unless the function gets used directly.

    inputmf = mmap_file_new(ifn, MMAP_READ);
    mmap_open(&inputmf);

    if (inputmf.status == -1)
        error(ERROR_INPUT_FILE_MMAP);

    input = inputmf.handle;

    FILE *temp_out = fopen(ofn, "w+");
    fwrite(input, sizeof(char), inputmf.size, temp_out);
    fclose(temp_out);

    mmap_close(&inputmf);

    outputmf = mmap_file_new(ofn, MMAP_READWRITE);
    mmap_open(&outputmf);

    if (!outputmf.status)
        error(ERROR_OUTPUT_FILE_MMAP);

    output = outputmf.handle;

    while (patch < patchend - 4)
    {
        uint32_t offset = patch32();
        uint16_t size = patch16();

        uint8_t *outputoff = (output + offset);

        if (size)
        {
            while (size--)
                *(outputoff++) = patch8();
        }
        else
        {
            size = patch16();
            uint8_t byte = patch8();

            while (size--)
                *(outputoff++) = byte;
        }
    }

    if (patch8() != 'E' || patch8() != 'E' || patch8() != 'O' || patch8() != 'F')
        error(IPS_NO_FOOTER);

#undef error
#undef patch8
#undef patch16
#undef patch32

end:
    mmap_close(&patchmf);
    mmap_close(&inputmf);
    mmap_close(&outputmf);

    return ret;
}
