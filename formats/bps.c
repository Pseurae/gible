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

enum bps_error
{
    BPS_SUCCESS = 0,
    BPS_INVALID_HEADER,
    BPS_TOO_SMALL,
    BPS_INVALID_ACTION,
    BPS_PATCH_CRC_NOMATCH,
    BPS_INPUT_CRC_NOMATCH,
    BPS_OUTPUT_CRC_NOMATCH,
    BPS_ERROR_COUNT
};

static const char *bps_error_messages[BPS_ERROR_COUNT];
static int bps_patch(patch_context_t *c);

static const char *bps_error_messages[BPS_ERROR_COUNT] = {
    [BPS_SUCCESS] = "BPS patching successful.",
    [BPS_INVALID_HEADER] = "Invalid header for a BPS file.",
    [BPS_TOO_SMALL] = "Patch file is too small to be a BPS file.",
    [BPS_INVALID_ACTION] = "Invalid BPS patching action.",
};

const patch_format_t bps_format = { "BPS", "BPS1", 4, bps_patch, bps_error_messages };

static int bps_patch(patch_context_t *c)
{
#define error(a, b, i) \
    if ((a)) { \
        if ((b)) { \
            fprintf(stderr, "%s\n", bps_error_messages[i]); \
        } else { \
            return i; \
        } \
    } \

#define patch8() (patch < patchend ? *(patch++) : 0)
#define input8() (input < inputend ? *(input++) : 0)
#define sign(b) ((b & 1 ? -1 : +1) * (b >> 1))

    int ret = BPS_SUCCESS;
    patch_flags_t *flags = c->flags;

    uint8_t *patch, *patchstart, *patchend, *patchcrc;
    uint8_t *input;
    uint8_t *output, *outputstart;

    uint32_t acrc[3] = {0, 0, 0};
    uint32_t scrc[3] = {0, 0, 0};

    if (c->patch.status == -1)
        return ERROR_PATCH_FILE_MMAP;

    if (c->patch.size < 19)
        return BPS_TOO_SMALL;

    patch = c->patch.handle;
    patchstart = patch;
    patchend = patch + c->patch.size;
    patchcrc = patchend - 12;

    if (~flags->ignore_crc & FLAG_CRC_PATCH)
    {
        scrc[2] = read32le(patchcrc + 8);
        acrc[2] = crc32(patchstart, c->patch.size - 4, 0);
        error(scrc[2] != acrc[2], flags->strict_crc & FLAG_CRC_PATCH, BPS_PATCH_CRC_NOMATCH);
    }

    if (patch8() != 'B' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        return BPS_INVALID_HEADER;

    size_t input_size = readvint(&patch);
    size_t output_size = readvint(&patch);

    c->input = mmap_file_new(c->fn.input, 1);
    mmap_open(&c->input);

    if (c->input.status == -1)
        return ERROR_INPUT_FILE_MMAP;

    input = c->input.handle;

    if (c->input.size != input_size)
        fprintf(stderr, "Input file sizes don't match.\n");

    if (~flags->ignore_crc & FLAG_CRC_INPUT)
    {
        scrc[0] = read32le(patchcrc);
        acrc[0] = crc32(input, input_size, 0);
        error(scrc[0] != acrc[0], flags->strict_crc & FLAG_CRC_INPUT, BPS_INPUT_CRC_NOMATCH);
    }

    c->output = mmap_file_new(c->fn.output, 0);
    mmap_create(&c->output, output_size);

    if (c->output.status == -1)
        return ERROR_OUTPUT_FILE_MMAP;

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
            return BPS_INVALID_ACTION;
        }
    }

    if (~flags->ignore_crc & FLAG_CRC_OUTPUT)
    {
        scrc[1] = read32le(patchcrc + 4);
        acrc[1] = crc32(c->output.handle, c->output.size, 0);
        error(scrc[1] != acrc[1], flags->strict_crc & FLAG_CRC_OUTPUT, BPS_OUTPUT_CRC_NOMATCH);
    }

#undef error
#undef patch8
#undef input8
#undef sign

    return ret;
}


