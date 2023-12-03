#include "qoi.h"
#include "stdlib.h"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define QOI_HEADER 0x66696F71
#define QOI_STREAM_END 0x0100000000000000
static __inline__ uint32_t reverse_u32_t(uint32_t value)
{
   return (value & 0x000000FF) << 24 | (value & 0x0000FF00) << 8 | (value & 0x00FF0000) >> 8 | (value & 0xFF000000) >> 24;
}
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error Endianess not supported
#else
#error Endianess not defined
#endif

struct qoi_header_t
{
    uint32_t header;
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

#define QOI_HEADER_SIZE 14

#define QOI_OP_TAG_MASK 0xc0

#define QOI_OP_INDEX_TAG 0x00
#define QOI_OP_DIFF_TAG 0x40
#define QOI_OP_LUMA_TAG 0x80
#define QOI_OP_RUN_TAG 0xc0
#define QOI_OP_RGB_TAG 0xfe
#define QOI_OP_RGBA_TAG 0xff

#define QOI_BUFFER_SIZE 64
#define QOI_MINIMUM_BUFFER_SIZE 1024
#define QOI_BUFFER_MINIMUM_FREE 5

size_t qoi_encode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output)
{
    uint8_t *buffer;
    size_t buffer_size = (size >> 1) > QOI_MINIMUM_BUFFER_SIZE ? size >> 1 : QOI_MINIMUM_BUFFER_SIZE;
    size_t buffer_index = 0;
    size_t read_index = 0;

    struct pixel_t previous_pixel = {.red = 0, .green = 0, .blue = 0, .alpha = 255};
    struct pixel_t pixel = previous_pixel;
    struct pixel_t *array = calloc(sizeof(struct pixel_t), QOI_BUFFER_SIZE);
    size_t run_count = 0;

    buffer = malloc(buffer_size);

    while (read_index < size)
    {
        if(buffer_size - buffer_index < QOI_BUFFER_MINIMUM_FREE)
        {
            size_t new_buffer_size = buffer_size + (buffer_size >> 1);
            buffer = (uint8_t *)realloc(buffer, new_buffer_size);
            buffer_size = new_buffer_size;
        }

        pixel.red = data[read_index];
        pixel.green = data[read_index + 1];
        pixel.blue = data[read_index + 2];
        pixel.alpha = (format == RGB) ? 255 : data[read_index + 3];
        read_index += format;

        if (pixel.red == previous_pixel.red && pixel.green == previous_pixel.green && pixel.blue == previous_pixel.blue && pixel.alpha == previous_pixel.alpha)
        {
            run_count++;
            if (run_count == 62)
            {
                buffer[buffer_index] = QOI_OP_RUN_TAG | (run_count - 1);
                buffer_index++;
                run_count = 0;
            }
            previous_pixel = pixel;
            continue;
        }
        if (run_count > 0)
        {
            buffer[buffer_index] = QOI_OP_RUN_TAG | (run_count - 1);
            buffer_index++;
            run_count = 0;
        }

        uint8_t hash = (pixel.red * 3 + pixel.green * 5 + pixel.blue * 7 + pixel.alpha * 11) % 64;
        if (pixel.red == array[hash].red && pixel.green == array[hash].green && pixel.blue == array[hash].blue && pixel.alpha == array[hash].alpha)
        {
            buffer[buffer_index] = QOI_OP_INDEX_TAG | hash;
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
                buffer_index += 2;
                previous_pixel = pixel;
                continue;
            }

            buffer[buffer_index] = QOI_OP_RGB_TAG;
            buffer[buffer_index + 1] = pixel.red;
            buffer[buffer_index + 2] = pixel.green;
            buffer[buffer_index + 3] = pixel.blue;
            buffer_index += 4;
            previous_pixel = pixel;
            continue;
        }

        buffer[buffer_index] = QOI_OP_RGBA_TAG;
        buffer[buffer_index + 1] = pixel.red;
        buffer[buffer_index + 2] = pixel.green;
        buffer[buffer_index + 3] = pixel.blue;
        buffer[buffer_index + 4] = pixel.alpha;
        buffer_index += 5;
        previous_pixel = pixel;
    }
    
    if (run_count != 0)
    {
        buffer[buffer_index] = QOI_OP_RUN_TAG | (run_count - 1);
        buffer_index++;
    }

    free(array);

    *output = buffer;
    return buffer_index;
}

size_t qoi_decode(const uint8_t *const data, const size_t size, const enum qoi_format_t format, uint8_t **output)
{
    (void)format;
    struct pixel_t previous_pixel = {.red = 0, .green = 0, .blue = 0, .alpha = 255};
    struct pixel_t pixel = previous_pixel;
    struct pixel_t *array = calloc(sizeof(struct pixel_t), QOI_BUFFER_SIZE);
    array[53] = previous_pixel;
    size_t input_index = 0;
    size_t output_index = 0;

    while (input_index < size)
    {
        uint64_t *data_end = (uint64_t *)(data + input_index);
        if(*data_end == QOI_STREAM_END)
        {
            printf("End of QOI stream\n");
            break;
        }

        uint8_t tag = data[input_index] & QOI_OP_TAG_MASK;
        uint8_t payload = data[input_index] & ~QOI_OP_TAG_MASK;

        switch (tag)
        {
        case QOI_OP_INDEX_TAG:
            pixel = array[payload];
            break;
        case QOI_OP_DIFF_TAG:
            pixel.red = previous_pixel.red + ((payload & 0x30) >> 4) - 2;
            pixel.green = previous_pixel.green + ((payload & 0x0c) >> 2) - 2;
            pixel.blue = previous_pixel.blue + (payload & 0x03) - 2;
            break;
        case QOI_OP_LUMA_TAG:
            pixel.green = previous_pixel.green + payload - 32;
            uint8_t deltas = data[input_index + 1];
            pixel.red = previous_pixel.red + ((deltas & 0xf0) >> 4) + payload - 40;
            pixel.blue = previous_pixel.blue + (deltas & 0x0f) + payload - 40;
            input_index++;
            break;
        case QOI_OP_RUN_TAG:
            if (payload < 62)
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
                break;
            }

            pixel.red = data[input_index + 1];
            pixel.green = data[input_index + 2];
            pixel.blue = data[input_index + 3];

            if (data[input_index] == QOI_OP_RGBA_TAG)
            {
                pixel.alpha = data[input_index + 4];
                input_index++;
            }
            input_index += 3;    
            break;
        }

        (*output)[output_index] = pixel.red;
        (*output)[output_index + 1] = pixel.green;
        (*output)[output_index + 2] = pixel.blue;
        (*output)[output_index + 3] = pixel.alpha;
        output_index += 4;

        uint8_t hash = (pixel.red * 3 + pixel.green * 5 + pixel.blue * 7 + pixel.alpha * 11) % 64;
        array[hash] = pixel;
        previous_pixel = pixel;
        input_index++;
    }

    return output_index;
}

struct image_t load_qoi(const char *const filename)
{
    FILE *qoi_ptr = fopen(filename, "rb");
    struct qoi_header_t header;
    struct image_t output = { .data=NULL, .size=0 };

    if (fread(&header, sizeof(header), 1, qoi_ptr) != 1)
    {
        printf("Failed to read QOI header\n");
        fflush(stdout);
        return output;
    }

    if (header.header != QOI_HEADER)
    {
        printf("File is not a QOI\n");
        fflush(stdout);
        return output;
    }
    output.width = reverse_u32_t(header.width);
    output.height = reverse_u32_t(header.height);
    output.format = header.channels;
    output.colorspace = header.colorspace;
    output.size = output.width * output.height * 4;
    output.data = malloc(output.size);

    fseek(qoi_ptr, 0L, SEEK_END);
    long filesize = ftell(qoi_ptr);
    if (filesize < 0)
    {
        printf("Error determining file size\n");
        fflush(stdout);
        return output;
    }

    uint8_t *buffer = malloc(filesize - QOI_HEADER_SIZE);
    fseek(qoi_ptr, QOI_HEADER_SIZE, SEEK_SET);
    fread(buffer, sizeof(uint8_t), filesize - QOI_HEADER_SIZE, qoi_ptr);
    fclose(qoi_ptr);

    qoi_decode(buffer, filesize - QOI_HEADER_SIZE, output.format, &output.data);

    free(buffer);

    return output;
}

void save_qoi(const struct image_t image_data, const char *const filename)
{
    (void)filename;

    uint8_t *data;
    size_t filesize = qoi_encode(image_data.data, image_data.size, image_data.format, &data);

    FILE *fp = fopen("qoi_test.qoi", "wb");

    uint32_t header = QOI_HEADER;
    fwrite(&header, sizeof(header), 1, fp);
    header = reverse_u32_t(image_data.width);
    fwrite(&header, sizeof(uint32_t), 1, fp);
    header  = reverse_u32_t(image_data.height);
    fwrite(&header, sizeof(uint32_t), 1, fp);
    fwrite(&(image_data.format), sizeof(uint8_t), 1, fp);
    fwrite(&(image_data.colorspace), sizeof(uint8_t), 1, fp);

    for(size_t i = 0; i < filesize; i++)
    {
        (void)fwrite(data + i, 1, 1, fp);
    }

    uint8_t temp;
    for (size_t i = 0; i < sizeof(QOI_STREAM_END); i++)
    {
        temp = QOI_STREAM_END >> (i * 8);
        (void)fwrite(&temp, 1, 1, fp);
    }

    (void)fclose(fp);

    free(data);
}