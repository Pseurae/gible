#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define gible_error(s, ...) \
{ \
    fprintf(stderr, s, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
}

#endif // LOG_H
