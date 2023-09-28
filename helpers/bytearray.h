#ifndef HELPERS_BYTEARRAY_H
#define HELPERS_BYTEARRAY_H

typedef struct bytearray
{
    unsigned long size;
    unsigned long capacity;
    unsigned char *data;
} bytearray_t;

bytearray_t bytearray_new(void);
int bytearray_resize(bytearray_t *a, unsigned long size);
void bytearray_push(bytearray_t *a, unsigned char c);
void bytearray_push_data(bytearray_t *a, unsigned char *byte, unsigned long size);
void bytearray_push_string(bytearray_t *a, const char *str);
void bytearray_push_vle(bytearray_t *a, unsigned long value);
void bytearray_close(bytearray_t *a);

#endif // HELPERS_BYTEARRAY_H