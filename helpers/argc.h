#ifndef HELPERS_ARGC_H
#define HELPERS_ARGC_H

#include <errno.h>  // errno, ERANGE
#include <stdio.h>  // fprintf, stderr, fputc
#include <stdlib.h> // strtol, strtof
#include <stdint.h> // uintptr_t
#include <string.h> // strcmp

#define ARGC_OPTION_FLAGS_INVERT          (1 << 0)

#define ARGC_PARSER_FLAGS_STOP_UNKNOWN    (1 << 0)
#define ARGC_PARSER_FLAGS_HELP_ON_UNKNOWN (1 << 1)
#define ARGC_PARSER_FLAGS_NO_POSITIONAL   (1 << 2)
#define ARGC_PARSER_FLAGS_IGNORE_UNKNOWN  (1 << 3)

#define ARGC_OPT_STRING(...) \
    { \
        ARGC_TYPE_STRING, __VA_ARGS__ \
    }

#define ARGC_OPT_FLAG(...) \
    { \
        ARGC_TYPE_FLAG, __VA_ARGS__ \
    }

#define ARGC_OPT_BOOLEAN(...) \
    { \
        ARGC_TYPE_BOOLEAN, __VA_ARGS__ \
    }

#define ARGC_OPT_INTEGER(...) \
    { \
        ARGC_TYPE_INTEGER, __VA_ARGS__ \
    }

#define ARGC_OPT_FLOAT(...) \
    { \
        ARGC_TYPE_FLOAT, __VA_ARGS__ \
    }

#define ARGC_OPT_END() \
    { \
        ARGC_TYPE_END, 0, NULL, NULL, 0, NULL, 0, NULL \
    }

#define ARGC_OPT_HELP() \
    { \
        ARGC_TYPE_CALLBACK, 'h', "help", NULL, 0, "Displays this message.", 0, argc_parser_help_callback \
    }

enum argc_type
{
    ARGC_TYPE_END,
    ARGC_TYPE_STRING,
    ARGC_TYPE_FLAG,
    ARGC_TYPE_BOOLEAN,
    ARGC_TYPE_INTEGER,
    ARGC_TYPE_FLOAT,
    ARGC_TYPE_CALLBACK,
};

struct argc_option;
struct argc_parser;
struct argc_ctx;

typedef struct argc_option argc_option_t;
typedef struct argc_parser argc_parser_t;
typedef struct argc_ctx argc_ctx_t;

typedef void (*argc_option_callback)(argc_parser_t *, const argc_option_t *);

struct argc_option
{
    enum argc_type type;
    const char sname;
    const char *lname;

    void *value_ptr;
    uintptr_t params;

    const char *desc;
    int flags;

    argc_option_callback callback;
};

struct argc_parser
{
    const char *execname;
    const struct argc_option *options;
    char **positional;
    int pcount;
    const char *desc;
    const char **usage;

    int flags;
};

struct argc_ctx
{
    int argc;
    char **argv;
    char *curropt;
    struct argc_parser *par;
};

argc_parser_t argc_parser_new(const char *execname, const argc_option_t *options, int flags);
void argc_parser_set_messages(argc_parser_t *par, const char *desc, const char **usage);
int argc_parser_execute(argc_ctx_t *ctx, const argc_option_t *option);
int argc_parser_short_opt(argc_ctx_t *ctx, const argc_option_t *options);
int argc_parser_long_opt(argc_ctx_t *ctx, const argc_option_t *options);
int argc_parser_print_usage(argc_parser_t *par);
int argc_parser_print_options_description(argc_parser_t *par);
int argc_parser_print_description(argc_parser_t *par);
int argc_parser_print_help(argc_parser_t *par);
void argc_parser_help_callback(argc_parser_t *par, const argc_option_t *option);
int argc_parser_parse(argc_parser_t *par, int argc, char **argv);

#endif /* HELPERS_ARGC_H */
