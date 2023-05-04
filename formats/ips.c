#include "../filemap.h"
#include "../format.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int ips_patch(patch_context_t *c);
const patch_format_t ips_format = { "IPS", "PATCH", 5, ips_patch };

static int ips_patch(patch_context_t *c)
{
    unsigned char *patch, *patchend, *input, *output;

    if (c->patch.status == -1)
        return PATCH_RET_INVALID_PATCH;

    if (c->patch.size < 8)
        return PATCH_ERROR("Patch file is too small to be an IPS file.");

    patch = c->patch.handle;
    patchend = patch + c->patch.size;

#define patch8()  ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch24() ((patch + 3 < patchend) ? (patch += 3, (patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    // Never gonna get called, unless the function gets used directly.
    if (patch8() != 'P' || patch8() != 'A' || patch8() != 'T' || patch8() != 'C' || patch8() != 'H')
        return PATCH_ERROR("Invalid header for an IPS file.");

    if (patchend[-3] != 'E' || patchend[-2] != 'O' || patchend[-1] != 'F')
        return PATCH_ERROR("EOF footer not found.");

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

    while (patch < patchend - 3)
    {
        unsigned int offset = patch24();
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
#undef patch24

    return PATCH_RET_SUCCESS;
}
