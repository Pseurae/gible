#include <stdio.h>
#include <stdint.h>

#include "../format.h"
#include "../utils.h"
#include "../filemap.h"
#include "../crc32.h"

enum bps_action
{
    BPS_SOURCE_READ,
    BPS_TARGET_READ,
    BPS_SOURCE_COPY,
    BPS_TARGET_COPY
};

static int bps_patch(patch_context_t *c);
const patch_format_t bps_format = { "BPS", "BPS1", 4, bps_patch };

static int bps_patch(patch_context_t *c)
{
#define check_crc32(c, a, b, err) \
    if ((a)) { \
        if ((b)) { \
            (void)(PATCH_ERROR(c, err)); \
        } else { \
            return PATCH_ERROR(c, err); \
        } \
    } \

#define patch8() (patch < patchend ? *(patch++) : 0)
#define input8() (input < inputend ? *(input++) : 0)
#define sign(b) ((b & 1 ? -1 : +1) * (b >> 1))

    patch_flags_t *flags = c->flags;

    unsigned char *patch, *patchstart, *patchend, *patchcrc;
    unsigned char *input;
    unsigned char *output, *outputstart;

    unsigned int acrc[3] = {0, 0, 0};
    unsigned int scrc[3] = {0, 0, 0};

    if (c->patch.status == -1)
        return PATCH_RET_INVALID_PATCH;

    if (c->patch.size < 19)
        return PATCH_ERROR(c, "Patch file is too small to be a BPS file.");

    patch = c->patch.handle;
    patchstart = patch;
    patchend = patch + c->patch.size;
    patchcrc = patchend - 12;

    if (~flags->ignore_crc & FLAG_CRC_PATCH)
    {
        scrc[2] = read32le(patchcrc + 8);
        acrc[2] = crc32(patchstart, c->patch.size - 4, 0);
        check_crc32(c, scrc[2] != acrc[2], flags->strict_crc & FLAG_CRC_PATCH, "Patch CRCs don't match.");
    }

    if (patch8() != 'B' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        return PATCH_ERROR(c, "Invalid header for a BPS file.");

    size_t input_size = readvint(&patch);
    size_t output_size = readvint(&patch);

    c->input = mmap_file_new(c->fn.input, 1);
    mmap_open(&c->input);

    if (c->input.status == -1)
        return PATCH_RET_INVALID_INPUT;

    input = c->input.handle;

    if (c->input.size != input_size)
        fprintf(stderr, "Input file sizes don't match.\n");

    if (~flags->ignore_crc & FLAG_CRC_INPUT)
    {
        scrc[0] = read32le(patchcrc);
        acrc[0] = crc32(input, input_size, 0);
        check_crc32(c, scrc[0] != acrc[0], flags->strict_crc & FLAG_CRC_INPUT, "Input CRCs don't match.");
    }

    c->output = mmap_file_new(c->fn.output, 0);
    mmap_create(&c->output, output_size);

    if (c->output.status == -1)
        return PATCH_RET_INVALID_OUTPUT;

    output = c->output.handle;
    outputstart = c->output.handle;

    size_t metadata_size = readvint(&patch);
    patch += metadata_size;

    size_t output_off = 0;
    size_t source_rel_off = 0;
    size_t target_rel_off = 0;

    while (patch < patchcrc)
    {
        size_t data = readvint(&patch);
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
            return PATCH_ERROR(c, "Invalid BPS patching action.");
        }
    }

    if (~flags->ignore_crc & FLAG_CRC_OUTPUT)
    {
        scrc[1] = read32le(patchcrc + 4);
        acrc[1] = crc32(c->output.handle, c->output.size, 0);
        check_crc32(c, scrc[1] != acrc[1], flags->strict_crc & FLAG_CRC_OUTPUT, "Output CRCs don't match.");
    }

#undef error
#undef patch8
#undef input8
#undef sign

    return PATCH_RET_SUCCESS;
}


