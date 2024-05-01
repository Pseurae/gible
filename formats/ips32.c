#include "helpers/bytearray.h"
#include "helpers/filemap.h"
#include "helpers/format.h"
#include <string.h> // memcpy

static int ips32_apply(patch_apply_context_t *c);
static int ips32_create_check(patch_create_context_t *c);
static int ips32_create(patch_create_context_t *c);
static int ips32_create_write(bytearray_t *b, unsigned char *patched, unsigned long patched_size, unsigned char *base, unsigned long base_size);
static int ips32_create_write_blocks(bytearray_t *b, unsigned int start, unsigned int end, unsigned char *patched, unsigned long patched_size, unsigned char *base, unsigned long base_size);


const patch_format_t ips32_format = 
{ 
    .name = "IPS32", 
    .header = "IPS32", 
    .ext = "ips", 
    .apply_main = ips32_apply, 
    .create_main = ips32_create, 
    .apply_check = NULL, 
    .create_check = ips32_create_check 
};

// -------------------------------------------------
// Patch Application
// -------------------------------------------------

static int ips32_apply(patch_apply_context_t *c)
{
    unsigned char *patch, *patchend, *input, *output;

    if (c->patch.size < 9)
        return APPLY_ERROR("Patch file is too small to be an IPS32 file.");

    patch = c->patch.handle;
    patchend = patch + c->patch.size;

#define patch8() ((patch < patchend) ? *(patch++) : 0)
#define patch16() ((patch + 2 < patchend) ? (patch += 2, (patch[-2] << 8 | patch[-1])) : 0)
#define patch32() \
    ((patch + 4 < patchend) ? (patch += 4, (patch[-4] << 24 | patch[-3] << 16 | patch[-2] << 8 | patch[-1])) : 0)

    // Never gonna get called, unless the function gets used directly.
    if (patch8() != 'I' || patch8() != 'P' || patch8() != 'S' || patch8() != '3' || patch8() != '2')
        return APPLY_ERROR("Invalid header for an IPS32 file.");

    if (patchend[-4] != 'E' || patchend[-3] != 'E' || patchend[-2] != 'O' || patchend[-1] != 'F')
        return APPLY_ERROR("EEOF footer not found.");

    input = c->input.handle;

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

// -------------------------------------------------
// Patch Creation
// -------------------------------------------------

static int ips32_create_check(patch_create_context_t *c)
{
    return c->patched.size > 0x1000000 && c->patched.size <= UINT32_MAX;
}

static void ips32_create_write_rle_block(bytearray_t *a, unsigned int address, unsigned short size, unsigned char byte)
{
    unsigned char *addressBytes = (unsigned char *)&address;
    unsigned char *sizeBytes = (unsigned char *)&size;

    bytearray_push(a, addressBytes[3]);
    bytearray_push(a, addressBytes[2]);
    bytearray_push(a, addressBytes[1]);
    bytearray_push(a, addressBytes[0]);

    bytearray_push(a, 0);
    bytearray_push(a, 0);

    bytearray_push(a, sizeBytes[1]);
    bytearray_push(a, sizeBytes[0]);

    bytearray_push(a, byte);
}

static void ips32_create_write_block(bytearray_t *a, unsigned int address, unsigned short size, unsigned char *bytes)
{
    unsigned char *addressBytes = (unsigned char *)&address;
    unsigned char *sizeBytes = (unsigned char *)&size;

    bytearray_push(a, addressBytes[3]);
    bytearray_push(a, addressBytes[2]);
    bytearray_push(a, addressBytes[1]);
    bytearray_push(a, addressBytes[0]);

    bytearray_push(a, sizeBytes[1]);
    bytearray_push(a, sizeBytes[0]);

    bytearray_push_data(a, bytes, size);
}

static int ips32_create(patch_create_context_t *c)
{
    bytearray_t b = bytearray_new();

    bytearray_push_string(&b, "IPS32");

    unsigned char *patched = c->patched.handle;
    unsigned long patched_size = c->patched.size;

    unsigned char *base = c->base.handle;
    unsigned long base_size = c->base.size;

    if (patched_size >= UINT32_MAX)
        return CREATE_ERROR("IPS cannot be used to patch files to size over 4.29GB.");

    ips32_create_write(&b, patched, patched_size, base, base_size);
    bytearray_push_string(&b, "EEOF");

    filemap_create(&c->output, b.size);
    memcpy(c->output.handle, b.data, b.size);

    bytearray_close(&b);

    return CREATE_RET_SUCCESS;
}

#define patched8(i) (patched[i])
#define base8(i) (i < base_size ? base[i] : 0)
#define changed(i) (patched8(i) != base8(i))
#define checkoffsize(off, start) ((off) < patched_size)

static int ips32_create_write_blocks(bytearray_t *b, unsigned int start, unsigned int end, unsigned char *patched, unsigned long patched_size, unsigned char *base, unsigned long base_size)
{
    if (start >= end) return 0;
    unsigned int length = end - start >= UINT16_MAX ? UINT16_MAX : end - start;

    unsigned int consecutive = 0;

    for (; patched8(start + consecutive) == patched8(start + consecutive + 1) && consecutive < length; ++consecutive)
        ;

    // The size of a RLE Block Header is 8 bytes.
    if (consecutive == length || consecutive > 8)
    {
        ips32_create_write_rle_block(b, start, consecutive, patched[start]);
        return ips32_create_write_blocks(b, start + consecutive, end, patched, patched_size, base, base_size);
    }
    else
    {
        unsigned int consecutive = 0, rleStart = 0;

        while (consecutive + rleStart < length)
        {
            if (patched8(start + consecutive + rleStart) == patched8(start + consecutive + rleStart + 1))
            {
                ++consecutive;
            }
            else
            {
                ++rleStart;
                consecutive = 0;
            }

            if ((consecutive > 8 + 5) ||
                (consecutive > 8 && rleStart + consecutive >= length))
            {
                if (rleStart) length = rleStart;
                break;
            }
        }

        unsigned int blockLength = length;
        while (patched8(start + blockLength - 1) == base8(start + blockLength - 1)) --blockLength;

        ips32_create_write_block(b, start, blockLength, patched + start);
        return ips32_create_write_blocks(b, start + length, end, patched, patched_size, base, base_size);
    }

    // ips32_create_write_block(b, start, length, patched + start);
    // return ips32_create_write_blocks(b, start + length, end, patched, patched_size, base, base_size);
}

static int ips32_create_write(bytearray_t *b, unsigned char *patched, unsigned long patched_size, unsigned char *base, unsigned long base_size)
{

    for (unsigned int offset = 0, start = 0, unchanged = 0; offset < patched_size; ++offset)
    {
        if (patched8(offset) == base8(offset))
            continue;

        if (memcmp(&offset, "EEOF", 4) == 0)
            offset--;

        start = offset;

        while (offset < patched_size)
        {
            for (; changed(offset) && checkoffsize(offset, start); ++offset)
                ;

            for (unchanged = 0; !changed(offset + unchanged) && checkoffsize(offset + unchanged, start); ++unchanged)
                ;

            // The size of a normal IPS Block Header is 5 bytes
            if (unchanged >= 5) break;

            offset += unchanged + 1;
            continue;
        }

        ips32_create_write_blocks(b, start, offset, patched, patched_size, base, base_size);
    }

    return CREATE_RET_SUCCESS;
}

#undef patched8
#undef base8
#undef changed8
#undef checkoffsize
