#include "qoi.h"
#include "stdlib.h"

struct qoi_header_t
{
    const char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colorspace;
};

struct pixel_t
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

#define QOI_OP_TAG_MASK 0xc0

#define QOI_OP_INDEX_TAG 0x00
#define QOI_OP_DIFF_TAG 0x40
#define QOI_OP_LUMA_TAG 0x80
#define QOI_OP_RUN_TAG 0xc0
#define QOI_OP_RGB_TAG 0xfe
#define QOI_OP_RGBA_TAG 0xff

#define ARRAY_SIZE 64

size_t qoi_encode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output)
{
    struct qoi_header_t header = {.magic = "qoif"};
    uint8_t stream_end[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    uint8_t *buffer = malloc(sizeof(header) + size + sizeof(stream_end));
    size_t buffer_index = 0;
    size_t read_index = 0;

    struct pixel_t previous_pixel = {.red = 0, .green = 0, .blue = 0, .alpha = 255};
    struct pixel_t pixel = previous_pixel;
    struct pixel_t *array = calloc(sizeof(struct pixel_t), ARRAY_SIZE);
    array[53] = previous_pixel;
    size_t run_count = 0;

    while (read_index < size)
    {
        printf("%lu: ", read_index);
        pixel.red = data[read_index];
        pixel.green = data[read_index + 1];
        pixel.blue = data[read_index + 2];
        pixel.alpha = (format == RGB) ? 255 : data[read_index + 3];
        read_index += format;
        printf("%u %u %u %u - ", pixel.red, pixel.green, pixel.blue, pixel.alpha);
        if (pixel.red == previous_pixel.red && pixel.green == previous_pixel.green && pixel.blue == previous_pixel.blue && pixel.alpha == previous_pixel.alpha)
        {
            run_count++;
            if (run_count == 62)
            {
                buffer[buffer_index] = QOI_OP_RUN_TAG | (run_count - 1);
                buffer_index++;
                run_count = 0;
            }
            printf("RUN\t%lu\n", run_count);
            previous_pixel = pixel;
            continue;
        }
        else if (run_count > 0)
        {
            buffer[buffer_index] = QOI_OP_RUN_TAG | (run_count - 1);
            buffer_index++;
            run_count = 0;
        }

        uint8_t hash = (pixel.red * 3 + pixel.green * 5 + pixel.blue * 7 + pixel.alpha * 11) % 64;
        printf("hash %u - ", hash);
        if (pixel.red == array[hash].red && pixel.green == array[hash].green && pixel.blue == array[hash].blue && pixel.alpha == array[hash].alpha)
        {
            buffer[buffer_index] = QOI_OP_INDEX_TAG | hash;
            printf("INDEX\t%u\n", hash);
            buffer_index++;

            previous_pixel = pixel;
            continue;
        }

        array[hash] = pixel;

        if (pixel.alpha == previous_pixel.alpha)
        {
            uint8_t delta_red = pixel.red - previous_pixel.red + 2;
            uint8_t delta_green = pixel.green - previous_pixel.green + 2;
            uint8_t delta_blue = pixel.blue - previous_pixel.blue + 2;

            if (delta_red < 4 && delta_green < 4 && delta_blue < 4)
            {
                buffer[buffer_index] = QOI_OP_DIFF_TAG | (delta_red << 4) | (delta_green << 2) | delta_blue;
                printf("DIFF\t%d %d %d\n", ((buffer[buffer_index] & 0x30) >> 4) - 2, ((buffer[buffer_index] & 0x0c) >> 2) - 2, (buffer[buffer_index] & 0x03) - 2);
                buffer_index++;
                previous_pixel = pixel;
                continue;
            }

            delta_green += 30;
            uint8_t dr_dg = delta_red - delta_green + 38;
            uint8_t db_dg = delta_blue - delta_green + 38;

            if (delta_green < 64 && dr_dg < 16 && db_dg < 16)
            {
                buffer[buffer_index] = QOI_OP_LUMA_TAG | delta_green;
                buffer[buffer_index + 1] = (dr_dg << 4) | db_dg;
                printf("LUMA\t%d %d %d\n", (buffer[buffer_index] & 0x3f) - 32, ((buffer[buffer_index + 1] & 0xf0) >> 4) - 8, (buffer[buffer_index + 1] & 0x0f) - 8);
                buffer_index += 2;
                previous_pixel = pixel;
                continue;
            }
        }

        if (format == RGB)
        {
            buffer[buffer_index] = QOI_OP_RGB_TAG;
            buffer[buffer_index + 1] = pixel.red;
            buffer[buffer_index + 2] = pixel.green;
            buffer[buffer_index + 3] = pixel.blue;
            printf("RGB\n");
            buffer_index += 4;
            previous_pixel = pixel;
        }
        else
        {
            buffer[buffer_index] = QOI_OP_RGBA_TAG;
            buffer[buffer_index + 1] = pixel.red;
            buffer[buffer_index + 2] = pixel.green;
            buffer[buffer_index + 3] = pixel.blue;
            buffer[buffer_index + 4] = pixel.alpha;
            printf("RGBA\n");
            buffer_index += 5;
            previous_pixel = pixel;
        }
    }

    printf("%c%c%c%c\n", header.magic[0], header.magic[1], header.magic[2], header.magic[3]);
    free(array);

    for (size_t i = 0; i < buffer_index; i++)
    {
        printf("%02x ", buffer[i]);
    }

    *output = buffer;
    return buffer_index;
}

size_t qoi_decode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output)
{
    (void)format;
    struct pixel_t previous_pixel = {.red = 0, .green = 0, .blue = 0, .alpha = 255};
    struct pixel_t pixel = previous_pixel;
    struct pixel_t *array = calloc(sizeof(struct pixel_t), ARRAY_SIZE);
    array[53] = previous_pixel;
    size_t input_index = 0;
    size_t output_index = 0;

    while (input_index < size)
    {
        uint8_t tag = data[input_index] & QOI_OP_TAG_MASK;
        uint8_t payload = data[input_index] & ~QOI_OP_TAG_MASK;
        printf("%lu: ", input_index);
        
        switch (tag)
        {
        case QOI_OP_INDEX_TAG:
            pixel = array[payload];
            printf("INDEX - %u, %u %u %u %u", payload, pixel.red, pixel.green, pixel.blue, pixel.alpha);
            break;
        case QOI_OP_DIFF_TAG:
            pixel.red = previous_pixel.red + ((payload & 0x30) >> 4) - 2;
            pixel.green = previous_pixel.green + ((payload & 0x0c) >> 2) - 2;
            pixel.blue = previous_pixel.blue + (payload & 0x03) - 2;
            printf("DIFF - %d %d %d", ((payload & 0x30) >> 4) - 2, ((payload & 0x0c) >> 2) - 2, (payload & 0x03) - 2);
            break;
        case QOI_OP_LUMA_TAG:
            pixel.green = previous_pixel.green + payload - 32;
            uint8_t deltas = data[input_index + 1];
            pixel.red = previous_pixel.red + ((deltas & 0xf0) >> 4) + payload - 40;
            pixel.blue = previous_pixel.blue + (deltas & 0x0f) + payload - 40;
            input_index++;
            printf("LUMA - %d %d %d", payload - 32, ((deltas & 0xf0) >> 4) + payload - 40, (deltas & 0x0f) + payload - 40);
            break;
        case QOI_OP_RUN_TAG:
            if (payload < 63)
            {
                pixel = previous_pixel;
                for (uint8_t i = 0; i < payload; i++)
                {
                    (*output)[output_index] = pixel.red;
                    (*output)[output_index + 1] = pixel.green;
                    (*output)[output_index + 2] = pixel.blue;
                    (*output)[output_index + 3] = pixel.alpha;
                    output_index += 4;
                }
                printf("RUN - %u", payload + 1);
                break;
            }

            pixel.red = data[input_index + 1];
            pixel.green = data[input_index + 2];
            pixel.blue = data[input_index + 3];
            input_index += 3;

            if (data[input_index] == QOI_OP_RGBA_TAG)
            {
                input_index++;
                pixel.alpha = data[input_index];
                printf("RGBA - %u %u %u %u", pixel.red, pixel.green, pixel.blue, pixel.alpha);
                break;
            }
            break;
        }

        (*output)[output_index] = pixel.red;
        (*output)[output_index + 1] = pixel.green;
        (*output)[output_index + 2] = pixel.blue;
        (*output)[output_index + 3] = pixel.alpha;
        output_index += 4;

        uint8_t hash = (pixel.red * 3 + pixel.green * 5 + pixel.blue * 7 + pixel.alpha * 11) % 64;
        printf(" ... hash: %u\n", hash);
        array[hash] = pixel;
        previous_pixel = pixel;
        input_index++;
    }

    return output_index;
}