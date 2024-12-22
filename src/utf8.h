#ifndef UTF8_H

#include <stdint.h>

typedef struct U8Point {
    uint32_t val;
    int bytes;
} U8Point;

int str_to_u8_point(char *in, U8Point *point);
int str_from_u8_point(char out[6], U8Point *point);

#define ERR_UTF8_POINTER    "expected pointer to utf-8 point"

#define UTF8_H
#endif

