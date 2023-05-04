#include "../crc32.h"
#include "../filemap.h"
#include "../format.h"
#include "../utils.h"
#include <stdint.h>
#include <stdio.h>

static int ups_patch(patch_context_t *c);
const patch_format_t ups_format = { "UPS", "UPS1", 4, ups_patch };

static int ups_patch(patch_context_t *c)
{
#define check_crc32(a, b, err) \
    if ((a)) \
    { \
        if ((b)) \
        { \
            return PATCH_ERROR(err); \
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

    if (c->patch.status == -1)
        return PATCH_RET_INVALID_PATCH;

    if (c->patch.size < 18)
        return PATCH_ERROR("Patch file is too small to be an UPS file.");

    patch = c->patch.handle;
    patchstart = patch;
    patchend = patch + c->patch.size;
    patchcrc = patchend - 12;

    if (~flags->ignore_crc & FLAG_CRC_PATCH)
    {
        scrc[2] = read32le(patchcrc + 8);
        acrc[2] = crc32(patchstart, c->patch.size - 4, 0);

        check_crc32(scrc[2] != acrc[2], flags->strict_crc & FLAG_CRC_PATCH, "Patch CRCs don't match.");
    }

    if (patch8() != 'U' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        return PATCH_ERROR("Invalid header for an UPS file.");

    unsigned long input_size = readvint(&patch);
    unsigned long output_size = readvint(&patch);

    c->input = mmap_file_new(c->fn.input, 1);
    mmap_open(&c->input);

    if (c->input.status == -1)
        return PATCH_RET_INVALID_INPUT;

    input = c->input.handle;
    inputend = input + c->input.size;

    if (c->input.size != input_size)
        gible_info("Input file sizes don't match.\n");

    if (~flags->ignore_crc & FLAG_CRC_INPUT)
    {
        scrc[0] = read32le(patchcrc);
        acrc[0] = crc32(input, input_size, 0);

        check_crc32(scrc[0] != acrc[0], flags->strict_crc & FLAG_CRC_INPUT, "Input CRCs don't match.");
    }

    c->output = mmap_file_new(c->fn.output, 0);
    mmap_create(&c->output, output_size);

    if (c->output.status == -1)
        return PATCH_RET_INVALID_OUTPUT;

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

    if (~flags->ignore_crc & FLAG_CRC_OUTPUT)
    {
        scrc[1] = read32le(patchcrc + 4);
        acrc[1] = crc32(c->output.handle, c->output.size, 0);
        check_crc32(scrc[1] != acrc[1], flags->strict_crc & FLAG_CRC_OUTPUT, "Output CRCs don't match.");
    }

#undef check_crc32
#undef patch8
#undef input8
#undef writeout8

    return PATCH_RET_SUCCESS;
}
