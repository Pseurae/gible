#ifndef LOG_H
#define LOG_H

#include <stdio.h>

enum log_level
{
    LOG_LVL_MSG,
    LOG_LVL_INFO,
    LOG_LVL_WARN,
    LOG_LVL_ERROR,
};

#define gible_msg(...)   gible_log(LOG_LVL_MSG, __VA_ARGS__)
#define gible_info(...)  gible_log(LOG_LVL_INFO, __VA_ARGS__)
#define gible_warn(...)  gible_log(LOG_LVL_WARN, __VA_ARGS__)
#define gible_error(...) gible_log(LOG_LVL_ERROR, __VA_ARGS__)

void gible_log(int level, const char *fmt, ...);

#endif // LOG_H
