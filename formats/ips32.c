#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../format.h"
#include "../filemap.h"

enum ips32_error
{
    IPS32_SUCCESS = 0,
    IPS32_INVALID_HEADER,
    IPS32_TOO_SMALL,
    IPS32_NO_FOOTER,
    IPS32_ERROR_COUNT
};

static const char *ips32_error_messages[IPS32_ERROR_COUNT];
static int ips32_patch(patch_context_t *);

static const char *ips32_error_messages[IPS32_ERROR_COUNT] = {
    [IPS32_SUCCESS] = "IPS32 patching successful.",
    [IPS32_INVALID_HEADER] = "Invalid header for an IPS32 file.",
    [IPS32_TOO_SMALL] = "Patch file is too small to be an IPS32 file.",
    [IPS32_NO_FOOTER] = "EEOF footer not found."
};

const patch_format_t ips32_format = { "IPS32", "IPS32", 5, ips32_patch, ips32_error_messages };

static int ips32_patch(patch_context_t *c)
{
    uint8_t *patch, *patchend, *input, *output;

    if (c->patch.status == -1)
        return ERROR_PATCH_FILE_MMAP;

    if (c->patch.size < 8)
        return IPS32_TOO_SMALL;

    patch = c->patch.handle;
    patchend = patch + c->patch.size;

#define patch8() ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch32() ((patch + 4 < patchend) ? (patch += 4, (patch[-4] << 24 | patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    if (patch8() != 'I' || patch8() != 'P' || patch8() != 'S' || patch8() != '3' || patch8() != '2')
        return IPS32_INVALID_HEADER; // Never gonna get called, unless the function gets used directly.

    c->input = mmap_file_new(c->fn.input, 1);
    mmap_open(&c->input);

    if (c->input.status == -1)
        return ERROR_INPUT_FILE_MMAP;

    input = c->input.handle;

    c->output = mmap_file_new(c->fn.output, 0);
    mmap_create(&c->output, c->input.size);

    if (!c->output.status)
        return ERROR_OUTPUT_FILE_MMAP;

    output = c->output.handle;

    memcpy(output, input, c->output.size);

    mmap_close(&c->input);

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
        return IPS32_NO_FOOTER;

#undef patch8
#undef patch16
#undef patch32

    return IPS32_SUCCESS;
}

