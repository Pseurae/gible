#include "bytearray.h"
#include <stdlib.h>
#include <string.h>

static void bytearray_init(bytearray_t *a);
static void bytearray_grow_if(bytearray_t *a);

bytearray_t bytearray_new(void)
{
    bytearray_t a;
    bytearray_init(&a);
    return a;
}

static void bytearray_init(bytearray_t *a)
{
    a->size = 0;
    a->capacity = 0;
    a->data = NULL;
}

int bytearray_resize(bytearray_t *a, unsigned long size)
{
    unsigned char *data = (unsigned char *)realloc(a->data, size);

    if (data)
    {
        a->data = data;
        a->capacity = size;
        return 1;
    }

    return 0;
}

static void bytearray_grow_if(bytearray_t *a)
{
    if (a->capacity == 0)
        bytearray_resize(a, 10);

    else if (a->size >= a->capacity)
        bytearray_resize(a, a->capacity * 2);
}

inline void bytearray_push(bytearray_t *a, unsigned char c)
{
    bytearray_grow_if(a);
    a->data[a->size++] = c;
}

void bytearray_push_data(bytearray_t *a, unsigned char *bytes, unsigned long size)
{
    while (size--)
        bytearray_push(a, *(bytes++));
}

void bytearray_push_string(bytearray_t *a, const char *str)
{
    unsigned long size = strlen(str);
    bytearray_push_data(a, (unsigned char *)str, size);
}

void bytearray_push_vle(bytearray_t *a, unsigned long value)
{
    while (1)
    {
        unsigned long x = value & 0x7f;

        value >>= 7;
        if (value == 0)
        {
            bytearray_push(a, 0x80 | x);
            break;
        }
        bytearray_push(a, x);
        value--;
    }
}

void bytearray_close(bytearray_t *a)
{
    if (a->data)
        free(a->data);
}
