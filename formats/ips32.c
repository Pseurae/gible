#include "../filemap.h"
#include "../format.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int ips32_patch(patch_context_t *c);
const patch_format_t ips32_format = { "IPS32", "IPS32", 5, ips32_patch };

static int ips32_patch(patch_context_t *c)
{
    unsigned char *patch, *patchend, *input, *output;

    if (c->patch.status == -1)
        return PATCH_RET_INVALID_PATCH;

    if (c->patch.size < 8)
        return PATCH_ERROR("Patch file is too small to be an IPS32 file.");

    patch = c->patch.handle;
    patchend = patch + c->patch.size;

#define patch8()  ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch32() \
    ((patch + 4 < patchend) ? (patch += 4, (patch[-4] << 24 | patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    // Never gonna get called, unless the function gets used directly.
    if (patch8() != 'I' || patch8() != 'P' || patch8() != 'S' || patch8() != '3' || patch8() != '2')
        return PATCH_ERROR("Invalid header for an IPS32 file.");

    if (patchend[-4] != 'E' || patchend[-3] != 'E' || patchend[-2] != 'O' || patchend[-1] != 'F')
        return PATCH_ERROR("EEOF footer not found.");

    c->input = mmap_file_new(c->fn.input, 1);
    mmap_open(&c->input);

    if (c->input.status == -1)
        return PATCH_RET_INVALID_INPUT;

    input = c->input.handle;

    c->output = mmap_file_new(c->fn.output, 0);
    mmap_create(&c->output, c->input.size);

    if (!c->output.status)
        return PATCH_RET_INVALID_OUTPUT;

    output = c->output.handle;

    memcpy(output, input, c->output.size);

    mmap_close(&c->input);

    while (patch < patchend - 4)
    {
        unsigned int offset = patch32();
        unsigned short size = patch16();

        unsigned char *outputoff = (output + offset);

        if (size)
        {
            while (size--)
                *(outputoff++) = patch8();
        }
        else
        {
            size = patch16();
            unsigned char byte = patch8();

            while (size--)
                *(outputoff++) = byte;
        }
    }

#undef patch8
#undef patch16
#undef patch32

    return PATCH_RET_SUCCESS;
}
