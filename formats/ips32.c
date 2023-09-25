#include "../bytearray.h"
#include "../filemap.h"
#include "../format.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

static int ips32_apply(patch_apply_context_t *c);
static int ips32_create_check(patch_create_context_t *c);
static int ips32_create(patch_create_context_t *c);
const patch_format_t ips32_format = { "IPS32", "IPS32", "ips", ips32_apply, ips32_create, NULL, ips32_create_check };

#define EEOF_MARKER 0x45454F46

static int ips32_apply(patch_apply_context_t *c)
{
    unsigned char *patch, *patchend, *input, *output;

    if (c->patch.size < 8)
        return APPLY_ERROR("Patch file is too small to be an IPS32 file.");

    patch = c->patch.handle;
    patchend = patch + c->patch.size;

#define patch8()  ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch32() \
    ((patch + 4 < patchend) ? (patch += 4, (patch[-4] << 24 | patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    // Never gonna get called, unless the function gets used directly.
    if (patch8() != 'I' || patch8() != 'P' || patch8() != 'S' || patch8() != '3' || patch8() != '2')
        return APPLY_ERROR("Invalid header for an IPS32 file.");

    if (patchend[-4] != 'E' || patchend[-3] != 'E' || patchend[-2] != 'O' || patchend[-1] != 'F')
        return APPLY_ERROR("EEOF footer not found.");

    input = c->input.handle;

    c->output = filemap_new(c->fn.output, 0);

    if (!filemap_create(&c->output, c->input.size))
        return APPLY_RET_INVALID_OUTPUT;

    output = c->output.handle;

    memcpy(output, input, c->output.size);

    filemap_close(&c->input);

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

    return APPLY_RET_SUCCESS;
}

static int ips32_create_check_rle(unsigned char *bytes, unsigned long size)
{
    if (size <= 2)
        return 0;

    unsigned char prev = *(bytes++);
    size--;

    while (size--)
    {
        if (prev != *bytes)
            return 0;

        prev = *(bytes++);
    }

    return 1;
}

static void ips32_create_write_block(bytearray_t *a, unsigned char *patched, unsigned int start, unsigned int end)
{
    unsigned int address = start;
    unsigned short size = end - start;

    unsigned char *addressBytes = (unsigned char *)&address;
    unsigned char *sizeBytes = (unsigned char *)&size;

    if (address == EEOF_MARKER)
        (address--, size++);

    bytearray_push(a, addressBytes[3]);
    bytearray_push(a, addressBytes[2]);
    bytearray_push(a, addressBytes[1]);
    bytearray_push(a, addressBytes[0]);

    if (ips32_create_check_rle(patched + address, size))
    {
        bytearray_push(a, 0);
        bytearray_push(a, 0);

        bytearray_push(a, sizeBytes[1]);
        bytearray_push(a, sizeBytes[0]);

        bytearray_push(a, *(patched + address));
    }
    else
    {
        bytearray_push(a, sizeBytes[1]);
        bytearray_push(a, sizeBytes[0]);

        bytearray_push_data(a, patched + address, size);
    }
}

static int ips32_create_check(patch_create_context_t *c)
{
    return c->patched.size > 0x1000000;
}

static int ips32_create(patch_create_context_t *c)
{
    bytearray_t b = bytearray_new();

    bytearray_push_string(&b, "IPS32");

    unsigned char *patched = c->patched.handle;
    unsigned long patched_size = c->patched.size;

    unsigned char *base = c->base.handle;
    unsigned long base_size = c->base.size;

#define patched8(i) (patched[i])
#define base8(i)    (i < base_size ? base[i] : 0)

    for (unsigned int offset = 0, start = 0; offset < patched_size; ++offset)
    {
        if (patched8(offset) == base8(offset))
            continue;

        start = offset;

        for (; patched8(offset) != base8(offset) && offset < patched_size && (offset - start) < UINT16_MAX;
             ++offset)
            ;

        ips32_create_write_block(&b, patched, start, offset);
    }

    bytearray_push_string(&b, "EEOF");

#undef patched8
#undef base8

    c->output = filemap_new(c->fn.output, 0);
    filemap_create(&c->output, b.size);
    memcpy(c->output.handle, b.data, b.size);

    bytearray_close(&b);

    return CREATE_RET_SUCCESS;
}
