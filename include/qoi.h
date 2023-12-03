#include "stdint.h"
#include "stdio.h"

enum qoi_format_t
{
    RGB = 3,
    RGBA = 4
};

enum qoi_colorspace_t
{
    sRGB = 0,
    LINEAR = 1
};

struct image_t {
    uint32_t width;
    uint32_t height;
    enum qoi_format_t format;
    enum qoi_colorspace_t colorspace;
    uint8_t* data;
    size_t size;
};

size_t qoi_encode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output);
size_t qoi_decode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output);

struct image_t load_qoi(const char *const filename);
void save_qoi(const struct image_t image_data, const char *const filename);