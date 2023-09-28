#include "helpers/argc.h"
#include "helpers/format.h"
#include "helpers/strings.h"
#include "helpers/utils.h"
#include "actions/patch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *gible_patch_usage[] = {
    "patch <patch> <input> <output> [-tyui] [-fgjk] [-b] [-v]",
    NULL,
};

static const char *general_errors[] = {
    [APPLY_RET_SUCCESS] = "", // Not used
    [APPLY_RET_INVALID_PATCH] = "Cannot open the given patch file.",
    [APPLY_RET_INVALID_INPUT] = "Cannot open the given input file.",
    [APPLY_RET_INVALID_OUTPUT] = "Cannot open the given output file.",
};

static int patch(const char *pfn, const char *ifn, const char *ofn, patch_flags_t *flags);

int gible_patch(const char *execname, int argc, char *argv[])
{
    patch_flags_t flags;
    memset(&flags, 0, sizeof(patch_flags_t));

    // clang-format off

    const argc_option_t options[] = {
        ARGC_OPT_HELP(),
        ARGC_OPT_FLAG('t', "ignore-patch-crc", &flags.ignore_crc, FLAG_CRC_PATCH, "Ignores patch file crc.", 0, NULL),
        ARGC_OPT_FLAG('y', "ignore-input-crc", &flags.ignore_crc, FLAG_CRC_INPUT, "Ignores input file crc.", 0, NULL),
        ARGC_OPT_FLAG('u', "ignore-output-crc", &flags.ignore_crc, FLAG_CRC_OUTPUT, "Ignores output file crc.", 0, NULL),
        ARGC_OPT_FLAG('i', "ignore-crc", &flags.ignore_crc, FLAG_CRC_ALL, "Ignores all crc checks.", 0, NULL),
        ARGC_OPT_FLAG('f', "strict-patch-crc", &flags.strict_crc, FLAG_CRC_PATCH, "Aborts on patch crc mismatch.", 0, NULL),
        ARGC_OPT_FLAG('g', "strict-input-crc", &flags.strict_crc, FLAG_CRC_INPUT, "Aborts on input crc mismatch.", 0, NULL),
        ARGC_OPT_FLAG('j', "strict-output-crc", &flags.strict_crc, FLAG_CRC_OUTPUT, "Aborts on output crc mismatch (Not really useful).", 0, NULL),
        ARGC_OPT_FLAG('k', "strict-crc", &flags.strict_crc, FLAG_CRC_ALL, "Ignores all crc checks.", 0, NULL),
        ARGC_OPT_END(),
    };

    // clang-format on

    argc_parser_t parser =
        argc_parser_new(execname, options, ARGC_PARSER_FLAGS_STOP_UNKNOWN | ARGC_PARSER_FLAGS_HELP_ON_UNKNOWN);
    argc_parser_set_messages(&parser, gible_description, gible_patch_usage);

    if (!argc_parser_parse(&parser, argc, argv))
        return 1;

    if (parser.pcount < 3)
        return (argc_parser_print_usage(&parser), 1);

    char *pfn = parser.positional[0];
    char *ifn = parser.positional[1];
    char *ofn = parser.positional[2];

    int ret;
    if ((ret = are_filenames_same(pfn, ifn, ofn)))
        return (gible_error(same_filename_errors[ret - 1]), 1);

    if (!file_exists(pfn))
        return (gible_error("Patch file does not exist."), 1);

    if (!file_exists(ifn))
        return (gible_error("Input file does not exist."), 1);

    return patch(pfn, ifn, ofn, &flags);
}

static int patch(const char *pfn, const char *ifn, const char *ofn, patch_flags_t *flags)
{
    patch_apply_context_t c;

    c.fn.patch = pfn;
    c.fn.input = ifn;
    c.fn.output = ofn;

    c.flags = flags;

    c.patch = filemap_new(pfn, 1);
    c.input = filemap_new(ifn, 1);

    filemap_open(&c.input);
    filemap_open(&c.patch);

    if (c.patch.status == FILEMAP_ERROR)
        return (gible_error(general_errors[APPLY_RET_INVALID_PATCH]), 1);

    if (c.input.status == FILEMAP_ERROR)
        return (gible_error(general_errors[APPLY_RET_INVALID_INPUT]), 1);

    for (const patch_format_t *const *format = patch_formats; *format; format++)
    {
        const char *header = (*format)->header;

        if (strncmp((char *)c.patch.handle, header, strlen(header)) != 0)
            continue;

        if ((*format)->apply_check && !(*format)->apply_check(&c))
            continue;

        int return_code = (*format)->apply_main(&c);

        filemap_close(&c.patch);
        filemap_close(&c.input);
        filemap_close(&c.output);

        switch (return_code)
        {
        case 0:
            gible_msg("%s successfully patched.", (*format)->name);
            break;
        case -1:
            break;
        default:
            gible_error(general_errors[return_code]);
            break;
        }

        return return_code != APPLY_RET_SUCCESS;
    }

    filemap_close(&c.patch);
    gible_error("Unsupported Patch Type.");
    return 1;
}
