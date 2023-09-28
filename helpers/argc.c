#include "helpers/argc.h"

argc_parser_t argc_parser_new(const char *execname, const argc_option_t *options, int flags)
{
    argc_parser_t par;
    par.execname = execname;
    par.options = options;
    par.desc = NULL;
    par.usage = NULL;
    par.pcount = 0;
    par.flags = flags;
    return par;
}

void argc_parser_set_messages(argc_parser_t *par, const char *desc, const char **usage)
{
    par->desc = desc;
    par->usage = usage;
}

// Returns 1 on success, -1 on failure
int argc_parser_execute(argc_ctx_t *ctx, const argc_option_t *option)
{
#define error(...) (fprintf(stderr, __VA_ARGS__), -1)

    char *s = NULL;
    errno = 0;

    switch (option->type)
    {
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

    case ARGC_TYPE_CALLBACK:
        option->callback(ctx->par, option);
        break;

    default:
        return -1;
    }

#undef error

    return 1;
}

// Returns 0 on no match, 1 on success, -1 on failure
int argc_parser_short_opt(argc_ctx_t *ctx, const argc_option_t *options)
{
    for (; options->type != ARGC_TYPE_END; options++)
        if (ctx->curropt[0] == options->sname)
            return argc_parser_execute(ctx, options);
    return 0;
}

int argc_parser_long_opt(argc_ctx_t *ctx, const argc_option_t *options)
{
    for (; options->type != ARGC_TYPE_END; options++)
        if (!strcmp(ctx->curropt, options->lname))
            return argc_parser_execute(ctx, options);

    return 0;
}

int argc_parser_print_usage(argc_parser_t *par)
{
    if (par->usage)
    {
        fprintf(stderr, "usage: %s %s\n", par->execname, par->usage[0]);

        for (const char **i = par->usage + 1; *i; i++)
            fprintf(stderr, "       %s %s\n", par->execname, *i);
    }

    return 1;
}

int argc_parser_print_options_description(argc_parser_t *par)
{
    if (par->options)
    {
        fprintf(stderr, "options:\n");
        const argc_option_t *options = par->options;

        for (; options->type != ARGC_TYPE_END; options++)
        {
            fputc('\n', stderr);
            fprintf(stderr, "-%c, --%s\n\t%s\n", options->sname, options->lname, options->desc);
        }
    }
    return 1;
}

int argc_parser_print_description(argc_parser_t *par)
{
    if (par->desc)
        fprintf(stderr, "%s\n", par->desc);

    return 1;
}

int argc_parser_print_help(argc_parser_t *par)
{
    argc_parser_print_description(par);
    if (par->usage)
        fputc('\n', stderr);
    argc_parser_print_usage(par);
    if (par->options)
        fputc('\n', stderr);
    argc_parser_print_options_description(par);
    return 1;
}

void argc_parser_help_callback(argc_parser_t *par, const argc_option_t *option)
{
    argc_parser_print_help(par);
    (void)option; // Silence GCC warning
    exit(EXIT_SUCCESS);
}

int argc_parser_parse(argc_parser_t *par, int argc, char **argv)
{
    /*
    argc =- 1;
    argv =+ 1;
    */

    par->positional = argv;

    argc_ctx_t ctx;
    ctx.argc = argc;
    ctx.argv = argv;
    ctx.par = par;

    for (; ctx.argc; ctx.argc--, ctx.argv++)
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
        if (!(par->flags & ARGC_PARSER_FLAGS_IGNORE_UNKNOWN))
        {
            if (ctx.curropt)
                fprintf(stderr, "%s: illegal option -- %s.\n", par->execname, ctx.curropt);
            else
                fprintf(stderr, "%s: illegal option -- %s.\n", par->execname, *ctx.argv);
        }

        if (par->flags & ARGC_PARSER_FLAGS_HELP_ON_UNKNOWN)
            argc_parser_print_usage(par);

        if (par->flags & ARGC_PARSER_FLAGS_STOP_UNKNOWN)
            return 0;
    }

    return 1;
}
