#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "argc.h"
#include "log.h"
#include "utils.h"
#include "filemap.h"
#include "format.h"

#define HEADER_REGION_LEN 10

static const char *gible_description = "Yet another rom patcher.";
static const char *gible_usage[] = {
    "<patch> <input> <output> [-tyui] [-fgjk] [-b] [-v]",
    NULL,
};

static const char *mmap_errors[] = {
    "Cannot open the given patch file.",
    "Cannot open the given input file.",
    "Cannot open the given output file.",
};

int main(int argc, char *argv[]);
static int are_filenames_same(char *pfn, char *ifn, char *ofn);
static int gible_main(int argc, char *argv[]);
static int patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags);

int main(int argc, char *argv[])
{
    return gible_main(argc, argv);
}

static int are_filenames_same(char *pfn, char *ifn, char *ofn)
{
    if (!strcmp(pfn, ifn))
    {
        gible_error("Error: Patch and input filenames are the same.");
        return 1;
    }

    if (!strcmp(ifn, ofn))
    {
        gible_error("Error: Input and output filenames are the same.");
        return 1;
    }

    if (!strcmp(pfn, ofn))
    {
        gible_error("Error: Patch and output filenames are the same.");
        return 1;
    }

    return 0;
}

static int gible_main(int argc, char *argv[])
{

    patch_flags_t flags;
    memset(&flags, 0, sizeof(patch_flags_t));

    argc_option_t options[] = {
        ARGC_OPT_HELP(),
        ARGC_OPT_END(),
    };

    argc_parser_t parser = argc_parser_new(*argv, options, ARGC_PARSER_FLAGS_STOP_UNKNOWN | ARGC_PARSER_FLAGS_HELP_ON_UNKNOWN);
    argc_parser_set_messages(&parser, gible_description, gible_usage);

    if (!argc_parser_parse(&parser, argc - 1, argv + 1))
        return 1;

    if (parser.pcount < 3)
    {
        argc_parser_print_usage(&parser);
        return 1;
    }

    char *pfn = parser.positional[0];
    char *ifn = parser.positional[1];
    char *ofn = parser.positional[2];

    if (are_filenames_same(pfn, ifn, ofn))
        return 1;

    if (!file_exists(pfn))
    {
        gible_error("Patch file does not exist.");
        return 1;
    }

    if (!file_exists(ifn))
    {
        gible_error("Input file does not exist.");
        return 1;
    }

    return patch(pfn, ifn, ofn, &flags);
}

static const patch_format_t *patch_formats[] = {
    &ips_format,
    &ips32_format,
    &ups_format,
    &bps_format,
    NULL
};

static int patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags)
{
    patch_context_t c;

    c.fn.patch = pfn;
    c.fn.input = ifn;
    c.fn.output = ofn;

    c.patch = mmap_file_new(pfn, 1);
    c.flags = flags;

    mmap_open(&c.patch);

    for (const patch_format_t **format = patch_formats; *format; format++)
    {
        char *header = (*format)->header;
        uint8_t header_len = (*format)->header_len;

        if (memcmp(c.patch.handle, header, header_len))
            continue;

        int return_code = (*format)->main(&c);

        if (return_code < ERROR_OUTPUT_FILE_MMAP)
        {
            gible_error("%s\n", (*format)->error_msgs[return_code]);
        }
        else
        {
            gible_error("%s\n", mmap_errors[INT16_MAX - return_code - 1]);
        }

        mmap_close(&c.patch);
        mmap_close(&c.input);
        mmap_close(&c.output);

        if (return_code > 0)
            return 1;
        else
            return 0;
    }

    gible_error("Error: Unsupported Patch Type.");
    return 1;
}
