#include <stdio.h>
#include <stdint.h>

#include "../format.h"
#include "../utils.h"
#include "../filemap.h"
#include "../crc32.h"

enum ups_error
{
    UPS_SUCCESS = 0,
    UPS_INVALID_HEADER,
    UPS_TOO_SMALL,
    UPS_PATCH_CRC_NOMATCH,
    UPS_INPUT_CRC_NOMATCH,
    UPS_OUTPUT_CRC_NOMATCH,
    UPS_ERROR_COUNT
};

static const char *ups_error_messages[UPS_ERROR_COUNT];
static int ups_patch(patch_context_t *c);

static const char *ups_error_messages[UPS_ERROR_COUNT] = {
    [UPS_SUCCESS] = "UPS patching successful.",
    [UPS_INVALID_HEADER] = "Invalid header for an UPS file.",
    [UPS_TOO_SMALL] = "Patch file is too small to be an UPS file.",
    [UPS_PATCH_CRC_NOMATCH] = "Patch CRCs don't match.",
    [UPS_INPUT_CRC_NOMATCH] = "Input CRCs don't match.",
    [UPS_OUTPUT_CRC_NOMATCH] = "Output CRCs don't match.",
};

const patch_format_t ups_format = { "UPS", "UPS1", 4, ups_patch, ups_error_messages };

static int ups_patch(patch_context_t *c)
{
#define error(a, b, i) \
    if ((a)) { \
        if ((b)) { \
            fprintf(stderr, "%s\n", ups_error_messages[i]); \
        } else { \
            return i; \
        } \
    } \

#define patch8() (patch < patchend ? *(patch++) : 0)
#define input8() (input < inputend ? *(input++) : 0)
#define writeout8(b) (output < outputend ? *(output++) = b : 0)

    patch_flags_t *flags = c->flags;

    uint8_t *patch, *patchstart, *patchend, *patchcrc;
    uint8_t *input, *inputend, *output, *outputend;

    uint32_t acrc[3] = {0, 0, 0};
    uint32_t scrc[3] = {0, 0, 0};

    if (c->patch.status == -1)
        return ERROR_PATCH_FILE_MMAP;

    if (c->patch.size < 18)
        return UPS_TOO_SMALL;

    patch = c->patch.handle;
    patchstart = patch;
    patchend = patch + c->patch.size;
    patchcrc = patchend - 12;

    if (~flags->ignore_crc & FLAG_CRC_PATCH)
    {
        scrc[2] = read32le(patchcrc + 8);
        acrc[2] = crc32(patchstart, c->patch.size - 4, 0);

        error(scrc[2] != acrc[2], flags->strict_crc & FLAG_CRC_PATCH, UPS_PATCH_CRC_NOMATCH);
    }

    if (patch8() != 'U' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        return UPS_INVALID_HEADER;

    size_t input_size = readvint(&patch);
    size_t output_size = readvint(&patch);

    c->input = mmap_file_new(c->fn.input, 1);
    mmap_open(&c->input);

    if (c->input.status == -1)
        return ERROR_INPUT_FILE_MMAP;

    input = c->input.handle;
    inputend = input + c->input.size;

    if (c->input.size != input_size)
        fprintf(stderr, "Input file sizes don't match.\n");

    if (~flags->ignore_crc & FLAG_CRC_INPUT)
    {
        scrc[0] = read32le(patchcrc);
        acrc[0] = crc32(input, input_size, 0);

        error(scrc[0] != acrc[0], flags->strict_crc & FLAG_CRC_INPUT, UPS_INPUT_CRC_NOMATCH);
    }

    c->output = mmap_file_new(c->fn.output, 0);
    mmap_create(&c->output, output_size);

    if (c->output.status == -1)
        return ERROR_OUTPUT_FILE_MMAP;

    output = c->output.handle;
    outputend = output + c->output.size;

    while (patch < patchcrc)
    {
        size_t offset = readvint(&patch);
        while (offset--)
            writeout8(input8());

        uint8_t b;
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
        error(scrc[1] != acrc[1], flags->strict_crc & FLAG_CRC_OUTPUT, UPS_OUTPUT_CRC_NOMATCH);
    }

#undef error
#undef patch8
#undef input8
#undef writeout8

    return UPS_SUCCESS;
}

