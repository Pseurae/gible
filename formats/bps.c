#include <stdio.h>
#include <stdint.h>

#include "bps.h"
#include "../utils.h"
#include "../mmap.h"

static int try_bps_patch(char *pfn, char *ifn, char *ofn);

int bps_check(uint8_t *patch)
{
    char patch_header[4] = {'B', 'P', 'S', '1'};
    for (int i = 0; i < 4; i++)
    {
        if (patch_header[i] != patch[i])
            return 0;
    }
    return 1;
}

int bps_patch_main(char *pfn, char *ifn, char *ofn)
{
    int status = try_bps_patch(pfn, ifn, ofn);
    return 0;
}

enum bps_action
{
    BPS_SOURCE_READ,
    BPS_TARGET_READ,
    BPS_SOURCE_COPY,
    BPS_TARGET_COPY
};

static int try_bps_patch(char *pfn, char *ifn, char *ofn)
{
#define error(i)  \
    do            \
    {             \
        ret = i;  \
        goto end; \
    } while (0)

#define patch8() (patch < patchend ? *(patch++) : 0)
#define input8() (input < inputend ? *(input++) : 0)
#define writeout8(b) (output < outputend ? *(output++) = b : 0)
#define sign(b) ((b & 1 ? -1 : +1) * (b >> 1))

    int ret = BPS_SUCCESS;

    gible_mmap_file_t patchmf, inputmf, outputmf;
    uint8_t *patch, *patchstart, *patchend, *patchcrc;
    uint8_t *input, *inputstart, *inputend;
    uint8_t *output, *outputstart, *outputend;

    uint32_t actual_crc[3] = {0, 0, 0};
    uint32_t stored_crc[3] = {0, 0, 0};

    patchmf = gible_mmap_file_new(pfn, GIBLE_MMAP_READ);
    gible_mmap_open(&patchmf);

    if (patchmf.status == -1)
        error(BPS_PATCH_FILE_MMAP);

    if (patchmf.size < 19)
        error(BPS_TOO_SMALL);

    patch = patchmf.handle;
    patchstart = patch;
    patchend = patch + patchmf.size;
    patchcrc = patchend - 12;

    stored_crc[2] = read32le(patchcrc + 8);
    actual_crc[2] = crc32_calculate(patchstart, patchmf.size - 4);

    if (stored_crc[2] != actual_crc[2])
        error(BPS_PATCH_CRC_NOMATCH);

    if (patch8() != 'B' || patch8() != 'P' || patch8() != 'S' || patch8() != '1')
        error(BPS_INVALID_HEADER);

    size_t input_size = read_vint(&patch);
    size_t output_size = read_vint(&patch);

    inputmf = gible_mmap_file_new(ifn, GIBLE_MMAP_READ);
    gible_mmap_open(&inputmf);
    if (inputmf.status == -1)
        error(BPS_INPUT_FILE_MMAP);

    input = inputmf.handle;
    inputstart = inputmf.handle;
    inputend = input + inputmf.size;

    if (inputmf.size != input_size)
    {
        fprintf(stderr, "Input file sizes don't match.\n");
        fprintf(stderr, "%zu %zu\n", input_size, inputmf.size);
    }

    stored_crc[0] = read32le(patchcrc);
    actual_crc[0] = crc32_calculate(input, input_size);

    if (stored_crc[0] != actual_crc[0])
        error(BPS_INPUT_CRC_NOMATCH);

    FILE *temp_out = fopen(ofn, "w");
    fseek(temp_out, output_size - 1, SEEK_SET);
    fputc(0, temp_out);
    fclose(temp_out);

    outputmf = gible_mmap_file_new(ofn, GIBLE_MMAP_READWRITE);
    gible_mmap_open(&outputmf);

    if (outputmf.status == -1)
        error(BPS_OUTPUT_FILE_MMAP);

    output = outputmf.handle;
    outputstart = outputmf.handle;
    outputend = output + outputmf.size;

    stored_crc[1] = read32le(patchcrc + 4);

    size_t metadata_size = read_vint(&patch);
    patch += metadata_size;

    size_t output_off = 0;
    size_t source_rel_off = 0;
    size_t target_rel_off = 0;

    while (patch < patchcrc)
    {
        size_t data = read_vint(&patch);
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
            data = read_vint(&patch);
            source_rel_off += sign(data);

            while (length--)
                output[output_off++] = input[source_rel_off++];
            break;

        case BPS_TARGET_COPY:
            data = read_vint(&patch);
            target_rel_off += sign(data);

            while (length--)
                output[output_off++] = output[target_rel_off++];
            break;

        default:
            error(BPS_INVALID_ACTION);
        }
    }

#undef error
#undef patch8
#undef input8
#undef writeout8

end:
    gible_mmap_close(&patchmf);
    gible_mmap_close(&inputmf);
    gible_mmap_close(&outputmf);

    return ret;
}