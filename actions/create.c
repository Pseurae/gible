#include "actions/create.h"
#include "helpers/argc.h"
#include "helpers/format.h"
#include "helpers/strings.h"
#include "helpers/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *gible_create_usage[] = {
    "create <patched> <base> <output>",
    NULL,
};

static const char *general_errors[] = {
    [CREATE_RET_SUCCESS] = NULL, // Not used.
    [CREATE_RET_INVALID_PATCHED] = "Cannot open the given patched file.",
    [CREATE_RET_INVALID_BASE] = "Cannot open the given base file.",
    [CREATE_RET_INVALID_OUTPUT] = "Cannot open the given output file.",
};

static int create(const char *pfn, const char *bfn, const char *ofn);

int gible_create(const char *execname, int argc, char *argv[])
{
    const argc_option_t options[] = {
        ARGC_OPT_HELP(),
        ARGC_OPT_END(),
    };
    argc_parser_t parser =
        argc_parser_new(execname, options, ARGC_PARSER_FLAGS_STOP_UNKNOWN | ARGC_PARSER_FLAGS_HELP_ON_UNKNOWN);
    argc_parser_set_messages(&parser, gible_description, gible_create_usage);

    if (!argc_parser_parse(&parser, argc, argv))
        return 1;

    if (parser.pcount < 3)
        return (argc_parser_print_usage(&parser), 1);

    char *pfn = parser.positional[0];
    char *bfn = parser.positional[1];
    char *ofn = parser.positional[2];

    int ret;
    if ((ret = are_filenames_same(pfn, bfn, ofn)))
        return (gible_error(same_filename_errors[ret - 1]), 1);

    if (!file_exists(pfn))
        return (gible_error("Patched file does not exist."), 1);

    if (!file_exists(bfn))
        return (gible_error("Base file does not exist."), 1);

    return create(pfn, bfn, ofn);
}

static int check_extension(const char *fname, const char *ext)
{
    if (fname == NULL || ext == NULL)
        return 0;
    unsigned long length = strlen(fname), ext_len = strlen(ext);
    if (!length || !ext_len || length < ext_len)
        return 0;
    return strcmp(fname + length - ext_len, ext) == 0;
}

static int create(const char *pfn, const char *bfn, const char *ofn)
{
    patch_create_context_t c;

    c.patched = filemap_new(pfn, 1, filemap_mmap_api);
    c.base = filemap_new(bfn, 1, filemap_mmap_api);
    c.output = filemap_new(ofn, 0, filemap_mmap_api);

    filemap_open(&c.patched);
    filemap_open(&c.base);

    if (c.patched.status != FILEMAP_OK)
        return (gible_error(general_errors[CREATE_RET_INVALID_PATCHED]), 1);

    if (c.base.status != FILEMAP_OK)
        return (gible_error(general_errors[CREATE_RET_INVALID_BASE]), 1);

    for (const patch_format_t *const *format = patch_formats; *format; format++)
    {
        if (!check_extension(ofn, (*format)->ext))
            continue;

        if ((*format)->create_check && !(*format)->create_check(&c))
            continue;

        int return_code = (*format)->create_main(&c);

        filemap_close(&c.patched);
        filemap_close(&c.base);
        filemap_close(&c.output);

        switch (return_code)
        {
        case 0:
            gible_msg("%s successfully created.", (*format)->name);
            break;
        case -1:
            break;
        default:
            gible_error(general_errors[return_code]);
            break;
        }

        return return_code != CREATE_RET_SUCCESS;
    }

    filemap_close(&c.patched);
    filemap_close(&c.base);
    gible_error("Unsupported Patch Type.");
    return 1;
}
