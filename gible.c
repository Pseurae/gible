#include "gible.h"
#include "argc.h"
#include "create.h"
#include "filemap.h"
#include "format.h"
#include "log.h"
#include "patch.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct
{
    const char *subcommand;
    int (*func)(const char *, int, char *[]);
} commands[] = {
    { "patch",  gible_patch},
    {"create", gible_create},
};

const char gible_description[] = "Yet another rom patcher.";
static const char *gible_usage[] = {
    "[patch, create]",
    NULL,
};

int main(int argc, char *argv[])
{
    const char *execname = argv[0];

    argc_option_t options[] = {
        ARGC_OPT_END(),
    };

    argc_parser_t parser =
        argc_parser_new(execname, options, ARGC_PARSER_FLAGS_STOP_UNKNOWN | ARGC_PARSER_FLAGS_IGNORE_UNKNOWN);
    argc_parser_set_messages(&parser, gible_description, gible_usage);

    argc_parser_parse(&parser, argc - 1, argv + 1);
    if (parser.pcount < 1)
        return (argc_parser_print_usage(&parser), 1);

    for (unsigned long i = 0; i < ARRAY_COUNT(commands); ++i)
    {
        if (strcmp(commands[i].subcommand, argv[1]) == 0)
            return commands[i].func(execname, argc - 2, argv + 2);
    }

    return (argc_parser_print_usage(&parser), 1);
}
