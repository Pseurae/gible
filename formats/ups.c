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
static inline int ups_check(uint8_t *patch);
static int ups_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags);

const patch_format_t ups_patch_format = {"UPS", 4, ups_check, ups_patch, ups_error_messages};

static const char *ups_error_messages[UPS_ERROR_COUNT] = {
    [UPS_SUCCESS] = "UPS patching successful.",
    [UPS_INVALID_HEADER] = "Invalid header for an UPS file.",
    [UPS_TOO_SMALL] = "Patch file is too small to be an UPS file.",
    [UPS_PATCH_CRC_NOMATCH] = "Patch CRCs don't match.",
    [UPS_INPUT_CRC_NOMATCH] = "Input CRCs don't match.",
    [UPS_OUTPUT_CRC_NOMATCH] = "Output CRCs don't match.",
};

static inline int ups_check(uint8_t *patch)
{
    char patch_header[4] = {'U', 'P', 'S', '1'};

    for (int i = 0; i < 4; i++)
    {
        if (patch_header[i] != patch[i])
            return 0;
    }

    return 1;
}

static int ups_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags)
{
#define report(i) (fprintf(stderr, "%s\n", ups_error_messages[i]))
#define error(i)  \
    do            \
    {             \
        ret = i;  \
        goto end; \
    } while (0)

#define patch8() (patch < patchend ? *(patch++) : 0)
#define input8() (input < inputend ? *(input++) : 0)
#define writeout8(b) (output < outputend ? *(output++) = b : 0)

    int ret = UPS_SUCCESS;

    mmap_file_t patchmf, inputmf, outputmf;
    uint8_t *patch, *patchstart, *patchend, *patchcrc;
    uint8_t *input, *inputend, *output, *outputend;

    uint32_t actual_crc[3] = {0, 0, 0};
    uint32_t stored_crc[3] = {0, 0, 0};

    patchmf = mmap_file_new(pfn, MMAP_READ);
    mmap_open(&patchmf);

    if (!patchmf.status)
        error(ERROR_PATCH_FILE_MMAP);

    if (patchmf.size < 18)
        error(UPS_TOO_SMALL);

    patch = patchmf.handle;
    patchstart = patch;
    patchend = patch + patchmf.size;
    patchcrc = patchend - 12;

    if (~flags->ignore_crc & FLAG_CRC_PATCH)
    {
        stored_crc[2] = read32le(patchcrc + 8);
        actual_crc[2] = crc32(patchstart, patchmf.size - 4, 0);

        if (stored_crc[2] != actual_crc[2])
        {
            if (flags->strict_crc & FLAG_CRC_PATCH)
                error(UPS_PATCH_CRC_NOMATCH);
            else
                report(UPS_PATCH_CRC_NOMATCH);
        }
    }

    if (patch8() != 'U' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        error(UPS_INVALID_HEADER);

    size_t input_size = readvint(&patch);
    size_t output_size = readvint(&patch);

    inputmf = mmap_file_new(ifn, MMAP_READ);
    mmap_open(&inputmf);

    if (!inputmf.status)
        error(ERROR_INPUT_FILE_MMAP);

    input = inputmf.handle;
    inputend = input + inputmf.size;

    if (inputmf.size != input_size)
        fprintf(stderr, "Input file sizes don't match.\n");

    if (~flags->ignore_crc & FLAG_CRC_INPUT)
    {
        stored_crc[0] = read32le(patchcrc);
        actual_crc[0] = crc32(input, input_size, 0);

        if (stored_crc[0] != actual_crc[0])
        {
            if (flags->strict_crc & FLAG_CRC_INPUT)
                error(UPS_INPUT_CRC_NOMATCH);
            else
                report(UPS_INPUT_CRC_NOMATCH);
        }
    }

    outputmf = mmap_file_new(ofn, MMAP_WRITE);
    mmap_create(&outputmf, output_size);

    if (!outputmf.status)
        error(ERROR_OUTPUT_FILE_MMAP);

    output = outputmf.handle;
    outputend = output + outputmf.size;

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
        stored_crc[1] = read32le(patchcrc + 4);
        actual_crc[1] = crc32(outputmf.handle, outputmf.size, 0);

        if (stored_crc[1] != actual_crc[1])
        {
            if (flags->strict_crc & FLAG_CRC_OUTPUT)
                error(UPS_OUTPUT_CRC_NOMATCH);
            else
                report(UPS_OUTPUT_CRC_NOMATCH);
        }
    }

#undef report
#undef error
#undef patch8
#undef input8
#undef writeout8

end:
    mmap_close(&patchmf);
    mmap_close(&inputmf);
    mmap_close(&outputmf);

    return ret;
}
