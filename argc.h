#ifndef ARGC_LIB_H
#define ARGC_LIB_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ARGC_IMPLEMENTATION
#undef ARGC_IMPLEMENTATION

#include <stdio.h> // fprintf, stderr, fputc
#include <stdlib.h> // strtol, strtof
#include <string.h> // strcmp
#include <errno.h> // errno, ERANGE

#define ARGC_OPTION_FLAGS_INVERT (1 << 0)

#define ARGC_PARSER_FLAGS_STOP_UNKNOWN (1 << 0)
#define ARGC_PARSER_FLAGS_NO_POSITIONAL (1 << 1)

#define ARGC_OPT_STRING(...) \
    { ARGC_TYPE_STRING, __VA_ARGS__ }

#define ARGC_OPT_FLAG(...) \
    { ARGC_TYPE_FLAG, __VA_ARGS__ }

#define ARGC_OPT_BOOLEAN(...) \
    { ARGC_TYPE_BOOLEAN, __VA_ARGS__ }

#define ARGC_OPT_INTEGER(...) \
    { ARGC_TYPE_INTEGER, __VA_ARGS__ }

#define ARGC_OPT_FLOAT(...) \
    { ARGC_TYPE_FLOAT, __VA_ARGS__ }

#define ARGC_OPT_END() \
    { ARGC_TYPE_END, 0, NULL, NULL, 0, NULL, 0 }

enum argc_type
{
    ARGC_TYPE_END,
    ARGC_TYPE_STRING,
    ARGC_TYPE_FLAG,
    ARGC_TYPE_BOOLEAN,
    ARGC_TYPE_INTEGER,
    ARGC_TYPE_FLOAT,
};

struct argc_option
{
    enum argc_type type;
    const char sname;
    const char *lname;

    void *value_ptr;
    uintptr_t params;

    const char *desc;
    int flags;
};

struct argc_parser
{
    const char *execname;
    struct argc_option *options;
    char **positional;
    int pcount;
    const char *desc;
    const char *usage;

    int flags;
};

struct argc_ctx
{
    int argc;
    char **argv;
    char *curropt;
};

struct argc_parser argc_parser_new(const char *execname, struct argc_option *options, int flags)
{
    struct argc_parser par;
    par.execname = execname;
    par.options = options;
    par.desc = NULL;
    par.usage = NULL;
    par.pcount = 0;
    par.flags = flags;
    return par;
}

void argc_parser_set_messages(struct argc_parser *par, const char *desc, const char *usage)
{
    par->desc = desc;
    par->usage = usage;
}

// Returns 1 on success, -1 on failure
int argc_parser_execute(struct argc_ctx *ctx, struct argc_option *option)
{
#define error(...) (fprintf(stderr, __VA_ARGS__), -1)

    char *s = NULL;
    errno = 0;

    switch (option->type) {
        case ARGC_TYPE_STRING:
            if (ctx->argc > 1)
            {
                *(char **)option->value_ptr = *(++ctx->argv);
                ctx->argc--;
            }
            else
            {
                return error("%s argument requires an string value.\n", option->lname);
            }
            break; 

        case ARGC_TYPE_FLAG:
            if (option->flags & ARGC_OPTION_FLAGS_INVERT)
                *(int *)option->value_ptr &= ~option->params;
            else
                *(int *)option->value_ptr |= option->params;

            break;

        case ARGC_TYPE_BOOLEAN:
            if (option->flags & ARGC_OPTION_FLAGS_INVERT)
                *(int *)option->value_ptr = 0;
            else
                *(int *)option->value_ptr = 1;
            break; 

        case ARGC_TYPE_INTEGER:
            if (ctx->argc > 1)
            {
                *(int *)option->value_ptr = strtol(*(++ctx->argv), &s, 0);
                ctx->argc--;
            }
            else
            {
                return error("%s argument requires an integer value.\n", option->lname);
            }

            if (errno == ERANGE) 
                return error("Out of range.\n");

            // if (errno == EINVAL)
            if (s[0] != '\0') 
                return error("Not a number.\n");

            break; 

        case ARGC_TYPE_FLOAT:
            if (ctx->argc > 1)
            {
                *(float *)option->value_ptr = strtof(*(++ctx->argv), &s);
                ctx->argc--;
            }
            else
            {
                return error("%s argument requires an float value.\n", option->lname);
            }

            if (errno == ERANGE) 
                return error("Out of range.\n");

            // if (errno == EINVAL)
            if (s[0] != '\0') 
                return error("Not a number.\n");

            break; 

        default:
            return -1;
    }

#undef error

    return 1;
}

// Returns 0 on no match, 1 on success, -1 on failure
int argc_parser_short_opt(struct argc_ctx *ctx, struct argc_option *options)
{
    for (;options->type != ARGC_TYPE_END; options++)
        if (ctx->curropt[0] == options->sname)
            return argc_parser_execute(ctx, options);
    return 0;
}

int argc_parser_long_opt(struct argc_ctx *ctx, struct argc_option *options)
{
    for (;options->type != ARGC_TYPE_END; options++)
        if (!strcmp(ctx->curropt, options->lname)) 
            return argc_parser_execute(ctx, options);

    return 0;
}

int argc_parser_usage(struct argc_parser *par)
{
    if (par->usage)
        fprintf(stderr, "%s\n", par->usage);

    fputc('\n', stderr);

    if (par->desc)
        fprintf(stderr, "%s\n", par->desc);
    return 1;
}

int argc_parser_parse(struct argc_parser *par, int argc, char **argv)
{
    /*
    argc =- 1;
    argv =+ 1;
    */

    par->positional = argv;

    struct argc_ctx ctx;
    ctx.argc = argc;
    ctx.argv = argv;

    for (;ctx.argc; ctx.argc--, ctx.argv++)
    {
        char *arg = *ctx.argv;
        ctx.curropt = NULL;

        if (arg[0] != '-' || !arg[1]) // May be a positional value
        {
            if (par->flags & ARGC_PARSER_FLAGS_NO_POSITIONAL)
                return 0;

            par->positional[par->pcount++] = arg;
            continue;
        }

        if (arg[1] != '-') // Short hand
        {
            ctx.curropt = arg + 1;
            if (!argc_parser_short_opt(&ctx, par->options)) 
                goto unknown_opt;
        }

        if (arg[2]) // Long hand
        {
            ctx.curropt = arg + 2;
            if (!argc_parser_long_opt(&ctx, par->options)) 
                goto unknown_opt;
        }

        continue;

unknown_opt:
        if (ctx.curropt)
            fprintf(stderr, "%s: illegal option -- %s.\n", par->execname, ctx.curropt);
        else
            fprintf(stderr, "%s: illegal option -- %s.\n", par->execname, *ctx.argv);

        argc_parser_usage(par);
        if (par->flags & ARGC_PARSER_FLAGS_STOP_UNKNOWN)
            return 0;
    }

    return 1;
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* ARGC_LIB_H */