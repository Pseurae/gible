#include "../crc32.h"
#include "../filemap.h"
#include "../format.h"
#include "../utils.h"
#include <stdint.h>
#include <stdio.h>

enum bps_action
{
    BPS_SOURCE_READ,
    BPS_TARGET_READ,
    BPS_SOURCE_COPY,
    BPS_TARGET_COPY
};

static int bps_apply(patch_apply_context_t *c);
static int bps_create(patch_create_context_t *c);
const patch_format_t bps_format = { "BPS", "BPS1", "bps", bps_apply, bps_create, NULL, NULL };

static int bps_apply(patch_apply_context_t *c)
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

#define patch8() (patch < patchend ? *(patch++) : 0)
#define input8() (input < inputend ? *(input++) : 0)
#define sign(b)  ((b & 1 ? -1 : +1) * (b >> 1))

    patch_flags_t *flags = c->flags;

    unsigned char *patch, *patchstart, *patchend, *patchcrc;
    unsigned char *input;
    unsigned char *output, *outputstart;

    unsigned int acrc[3] = { 0, 0, 0 };
    unsigned int scrc[3] = { 0, 0, 0 };

    if (c->patch.size < 19)
        return APPLY_ERROR("Patch file is too small to be a BPS file.");

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

    if (patch8() != 'B' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        return APPLY_ERROR("Invalid header for a BPS file.");

    unsigned long input_size = readvint(&patch);
    unsigned long output_size = readvint(&patch);

    input = c->input.handle;

    if (c->input.size != input_size)
        gible_info("Input file sizes don't match.\n");

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
    outputstart = c->output.handle;

    unsigned long metadata_size = readvint(&patch);
    patch += metadata_size;

    unsigned long output_off = 0;
    unsigned long source_rel_off = 0;
    unsigned long target_rel_off = 0;

    while (patch < patchcrc)
    {
        unsigned long data = readvint(&patch);
        uint64_t action = data & 3;
        uint64_t length = (data >> 2) + 1;

        switch (action)
        {
        case BPS_SOURCE_READ:
            while (length--)
            {
                outputstart[output_off] = input[output_off];
                output_off++;
            }
            break;

        case BPS_TARGET_READ:
            while (length--)
                output[output_off++] = patch8();
            break;

        case BPS_SOURCE_COPY:
            data = readvint(&patch);
            source_rel_off += sign(data);

            while (length--)
                output[output_off++] = input[source_rel_off++];
            break;

        case BPS_TARGET_COPY:
            data = readvint(&patch);
            target_rel_off += sign(data);

            while (length--)
                output[output_off++] = output[target_rel_off++];
            break;

        default:
            return APPLY_ERROR("Invalid BPS patching action.");
        }
    }

    if (~flags->ignore_crc & FLAG_CRC_OUTPUT)
    {
        scrc[CRC_OUTPUT] = read32le(patchcrc + 4);
        acrc[CRC_OUTPUT] = crc32(c->output.handle, c->output.size, 0);
        check_crc32(CRC_OUTPUT, "Output CRCs don't match.");
    }

#undef check_crc32
#undef patch8
#undef input8
#undef sign

    return APPLY_RET_SUCCESS;
}

static int bps_create(patch_create_context_t *c)
{
    (void)c;
    return APPLY_ERROR("BPS patch creation not implemented yet.");
}
