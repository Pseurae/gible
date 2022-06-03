#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ARGC_IMPLEMENTATION
#include "argc.h"

#include "utils.h"
#include "mmap.h"
#include "format.h"
#include "formats/ips.h"
#include "formats/ups.h"
#include "formats/bps.h"

#define HEADER_REGION_LEN 50
#define ARRAY_COUNT(s) (sizeof(s) / sizeof(*s))

static const patch_format_t *patch_formats[] = {
    &ips_patch_format,
    &ips32_patch_format,
    &ups_patch_format,
    &bps_patch_format,
};

static void gible_main(int argc, char *argv[]);
static void gible_usage(const char *execname);
static int gible_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags);

int main(int argc, char *argv[])
{
    gible_main(argc, argv);
    return 0;
}

static int gible_check_filenames(char *pfn, char *ifn, char *ofn)
{
    if (!strcmp(pfn, ifn))
    {
        fprintf(stderr, "Error: Patch and input filenames are the same.\n");
        return 0;
    }

    if (!strcmp(ifn, ofn))
    {
        fprintf(stderr, "Error: Input and output filenames are the same.\n");
        return 0;
    }

    if (!strcmp(pfn, ofn))
    {
        fprintf(stderr, "Error: Patch and output filenames are the same.\n");
        return 0;
    }

    return 1;
}

static void gible_main(int argc, char *argv[])
{
    if (argc < 4)
    {
        gible_usage(*argv);
        exit(EXIT_SUCCESS);
    }

    char *pfn = argv[1];
    char *ifn = argv[2];
    char *ofn = argv[3];

    if (!gible_check_filenames(pfn, ifn, ofn)) return;

    if (!file_exists(pfn))
    {
        fprintf(stderr, "Patch file does not exist.\n");
        return;
    }

    if (!file_exists(ifn))
    {
        fprintf(stderr, "Input file does not exist.\n");
        return;
    }

    patch_flags_t flags;
    memset(&flags, 0, sizeof(patch_flags_t));

    struct argc_option options[] = {
        ARGC_OPT_FLAG('t', "ignore-patch-crc", &flags.ignore_crc, FLAG_CRC_PATCH, "Ignores patch file crc.", 0),
        ARGC_OPT_FLAG('y', "ignore-input-crc", &flags.ignore_crc, FLAG_CRC_INPUT, "Ignores input file crc.", 0),
        ARGC_OPT_FLAG('u', "ignore-output-crc", &flags.ignore_crc, FLAG_CRC_OUTPUT, "Ignores output file crc.", 0),
        ARGC_OPT_BOOLEAN('i', "ignore-crc", &flags.ignore_crc, 0, "Ignores all crc checks.", 0),

        ARGC_OPT_FLAG('g', "strict-patch-crc", &flags.strict_crc, FLAG_CRC_PATCH, "Aborts on patch crc mismatch.", 0),
        ARGC_OPT_FLAG('h', "strict-input-crc", &flags.strict_crc, FLAG_CRC_INPUT, "Aborts on input crc mismatch.", 0),
        ARGC_OPT_FLAG('j', "strict-output-crc", &flags.strict_crc, FLAG_CRC_OUTPUT, "Aborts on output crc mismatch (Not much useful).", 0),
        ARGC_OPT_BOOLEAN('k', "strict-crc", &flags.strict_crc, 0, "Ignores all crc checks.", 0),

        ARGC_OPT_BOOLEAN('b', "prefer-filebuffer", &flags.verbose, 0, "Uses filebuffer mode over mmap.", 0),
        ARGC_OPT_BOOLEAN('v', "verbose", &flags.verbose, 0, "Verbose mode.", 0),
    };

    struct argc_parser parser = argc_parser_new(*argv, options, 0);
    argc_parser_set_messages(&parser, "Yet another rom patcher.", "");
    argc_parser_parse(&parser, argc - 4, argv + 4);

    int ret = gible_patch(pfn, ifn, ofn, &flags);

    if (ret == -1)
        fprintf(stderr, "Unsupported patch type.\n");

    return;
}

static void gible_usage(const char *execname)
{
    fprintf(
        stderr,
        "Usage:\n"
        "%s <patch> <input> <output>\n",
        execname);
}

static const char *mmap_errors[] = {
    "Cannot open the given patch file.",
    "Cannot open the given input file.",
    "Cannot open the given output file.",
};

static int gible_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags)
{
    uint8_t header_buf[HEADER_REGION_LEN];
    FILE *p = fopen(pfn, "r");
    fread(header_buf, sizeof(char), HEADER_REGION_LEN, p);
    fclose(p);

    for (uintmax_t i = 0; i < ARRAY_COUNT(patch_formats); i++)
    {
        const patch_format_t *format = patch_formats[i];

        if (format->check(header_buf))
        {
            int ret = format->main(pfn, ifn, ofn, flags);

            if (ret < ERROR_OUTPUT_FILE_MMAP)
                fprintf(stderr, "%s\n", format->error_msgs[ret]);
            else
                fprintf(stderr, "%s\n", mmap_errors[INT16_MAX - ret - 1]);

            return ret;
        }
    }

    return -1;
}
