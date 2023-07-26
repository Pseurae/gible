#include "../bytearray.h"
#include "../crc32.h"
#include "../filemap.h"
#include "../format.h"
#include "../utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int ups_apply(patch_apply_context_t *c);
static int ups_create(patch_create_context_t *c);
const patch_format_t ups_format = { "UPS", "UPS1", "ups", ups_apply, ups_create, NULL, NULL };

static int ups_apply(patch_apply_context_t *c)
{
#define check_crc32(a, err) \
    if ((scrc[a] != acrc[a])) \
    { \
        if ((flags->strict_crc & FLAG_##a)) \
        { \
            return APPLY_ERROR(err); \
        } \
        else \
        { \
            (gible_warn(err)); \
        } \
    }

#define patch8()     (patch < patchend ? *(patch++) : 0)
#define input8()     (input < inputend ? *(input++) : 0)
#define writeout8(b) (output < outputend ? *(output++) = b : 0)

    patch_flags_t *flags = c->flags;

    unsigned char *patch, *patchstart, *patchend, *patchcrc;
    unsigned char *input, *inputend, *output, *outputend;

    unsigned int acrc[3] = { 0, 0, 0 };
    unsigned int scrc[3] = { 0, 0, 0 };

    if (c->patch.size < 18)
        return APPLY_ERROR("Patch file is too small to be an UPS file.");

    patch = c->patch.handle;
    patchstart = patch;
    patchend = patch + c->patch.size;
    patchcrc = patchend - 12;

    if (~flags->ignore_crc & FLAG_CRC_PATCH)
    {
        scrc[CRC_PATCH] = read32le(patchcrc + 8);
        acrc[CRC_PATCH] = crc32(patchstart, c->patch.size - 4, 0);

        check_crc32(CRC_PATCH, "Patch CRCs don't match.");
    }

    if (patch8() != 'U' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        return APPLY_ERROR("Invalid header for an UPS file.");

    unsigned long input_size = readvint(&patch);
    unsigned long output_size = readvint(&patch);

    input = c->input.handle;
    inputend = input + c->input.size;

    if (c->input.size != input_size)
        gible_info("Input file sizes don't match.");

    if (~flags->ignore_crc & FLAG_CRC_INPUT)
    {
        scrc[CRC_INPUT] = read32le(patchcrc);
        acrc[CRC_INPUT] = crc32(input, input_size, 0);

        check_crc32(CRC_INPUT, "Input CRCs don't match.");
    }

    c->output = filemap_new(c->fn.output, 0);

    if (!filemap_create(&c->output, output_size))
        return APPLY_RET_INVALID_OUTPUT;

    output = c->output.handle;
    outputend = output + c->output.size;

    while (patch < patchcrc)
    {
        unsigned long offset = readvint(&patch);
        while (offset--)
            writeout8(input8());

        unsigned char b;
        do
        {
            b = patch8();
            writeout8(input8() ^ b);
        } while (b);
    }

    while (output < outputend && input < inputend)
        writeout8(input8());

    if (~flags->ignore_crc & FLAG_CRC_OUTPUT)
    {
        scrc[CRC_OUTPUT] = read32le(patchcrc + 4);
        acrc[CRC_OUTPUT] = crc32(c->output.handle, c->output.size, 0);
        check_crc32(CRC_OUTPUT, "Output CRCs don't match.");
    }

#undef check_crc32
#undef patch8
#undef input8
#undef writeout8

    return APPLY_RET_SUCCESS;
}

static int ups_create(patch_create_context_t *c)
{
    bytearray_t b = bytearray_new();

    bytearray_push_string(&b, "UPS1");

    unsigned char *patched = c->patched.handle;
    unsigned long patched_size = c->patched.size;

    unsigned char *base = c->base.handle;
    unsigned long base_size = c->base.size;

#define patched8(i) (patched[i])
#define base8(i)    (i < base_size ? base[i] : 0)
#define write32le(a, i) \
    (bytearray_push(a, i[0]), bytearray_push(a, i[1]), bytearray_push(a, i[2]), bytearray_push(a, i[3]))

    bytearray_push_vle(&b, base_size);
    bytearray_push_vle(&b, patched_size);

    for (unsigned long offset = 0, rel_offset = 0; offset < patched_size; offset++)
    {
        if (patched8(offset) == base8(offset))
            continue;

        bytearray_push_vle(&b, offset - rel_offset);

        for (; patched8(offset) != base8(offset) && offset < patched_size; ++offset)
            bytearray_push(&b, patched8(offset) ^ base8(offset));

        bytearray_push(&b, 0);
        rel_offset = offset + 1;
    }

    unsigned int crc_input = crc32(base, base_size, 0);
    unsigned int crc_output = crc32(patched, patched_size, 0);

    unsigned char *crc_input_bytes = (unsigned char *)&crc_input;
    unsigned char *crc_output_bytes = (unsigned char *)&crc_output;

    write32le(&b, crc_input_bytes);
    write32le(&b, crc_output_bytes);

    unsigned int crc_patch = crc32(b.data, b.size, 0);
    unsigned char *crc_patch_bytes = (unsigned char *)&crc_patch;

    write32le(&b, crc_patch_bytes);

#undef patched8
#undef base8
#undef write32le

    c->output = filemap_new(c->fn.output, 0);
    filemap_create(&c->output, b.size);
    memcpy(c->output.handle, b.data, b.size);

    bytearray_close(&b);

    return CREATE_RET_SUCCESS;
}
