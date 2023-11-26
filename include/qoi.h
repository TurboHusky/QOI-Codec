#include "stdint.h"
#include "stdio.h"

enum qoi_format_t
{
    RGB = 3,
    RGBA = 4
};

size_t qoi_encode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output);
size_t qoi_decode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output);