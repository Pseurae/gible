#include "helpers/log.h"
#include <stdarg.h>

void gible_log(int level, const char *fmt, ...)
{
    static const char *level_strings[] = { "", "INFO", "WARN", "ERROR" };

    va_list args;

    if (level != LOG_LVL_MSG)
        fprintf(stdout, "[%s] ", level_strings[level]);

    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}