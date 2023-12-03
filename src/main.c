#include "qoi.h"
#include <stdlib.h>

int main (int argc, char** argv)
{
    (void) argc;

    struct image_t tmp;
    tmp = load_qoi(argv[1]);
    printf("Loaded QOI:\t%u x %u px, %u channels, colorspace %u\n", tmp.width, tmp.height, tmp.channels, tmp.colorspace);

    // Test: Save intermediate images as .ppm
    FILE *fp = fopen("qoi_test.ppm", "wb");
    fprintf(fp, "P6\n%d %d\n255\n", tmp.width, tmp.height);
    if(tmp.channels == RGBA) {
        FILE *fp_a = fopen("qoi_alpha.ppm", "wb");
        fprintf(fp_a, "P6\n%d %d\n255\n", tmp.width, tmp.height);
        for (uint32_t i = 0; i < tmp.size; i += tmp.channels)
        {
            fwrite(tmp.data + i, sizeof(uint8_t), 3, fp);
            if(tmp.channels == RGBA)
            {
                for (int j = 0; j < 3; j++)
                {
                    fwrite(tmp.data + i + 3, sizeof(uint8_t), 1, fp_a);
                }
            }
        }
        fclose(fp_a);
    }
    else
    {
        fwrite(tmp.data, 1, tmp.size, fp);
    }
    fclose(fp);

    save_qoi(tmp, "qoi_test.qoi");

    free(tmp.data);

    return 0;
}