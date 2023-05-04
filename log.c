#include "log.h"
#include <stdarg.h>

void gible_error(const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    fprintf(stderr, "\n");
    va_end(arglist);
}