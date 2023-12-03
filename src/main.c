#include "qoi.h"
#include <stdlib.h>

int main (int argc, char** argv)
{
    (void) argc;

    struct image_t tmp;
    tmp = load_qoi(argv[1]);
        
    printf("Loaded QOI\n==========\n\n");
    printf("\t%u x %u px\n\t%u channels\n\t%u colorspace\n", tmp.width, tmp.height, tmp.format, tmp.colorspace);

    // Test output for debugging
    FILE *fp = fopen("qoi_test.ppm", "wb");
    FILE *fp_a = fopen("qoi_alpha.ppm", "wb");
    fprintf(fp, "P6\n%d %d\n255\n", tmp.width, tmp.height);
    fprintf(fp_a, "P6\n%d %d\n255\n", tmp.width, tmp.height);
    for (uint32_t i = 0; i < tmp.size; i += 4)
    {
        fwrite(tmp.data + i, 1, 1, fp);
        fwrite(tmp.data + i + 1, 1, 1, fp);
        fwrite(tmp.data + i + 2, 1, 1, fp);
        for (int j = 0; j < 3; j++)
        {
            fwrite(tmp.data + i + 3, 1, 1, fp_a);
        }
    }
    fclose(fp);
    fclose(fp_a);

    tmp.format = RGBA;
    save_qoi(tmp, "qoi_test.qoi");

    free(tmp.data);

    return 0;
}