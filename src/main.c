#include "qoi.h"
#include <stdlib.h>

int main (int argc, char** argv)
{
    (void) argc;
    (void) argv;

    uint8_t test_data[] = { 0,0,0,255, 0,0,0,255, 0,0,0,255, 0,255,0,255,
                            255,0,0,255, 255,0,0,255, 255,0,0,255, 255,0,0,255,
                            0,0,255,255, 0,0,255,127, 0,0,255,128, 0,0,255,255,
                            255,0,0,255, 255,255,0,255, 5,255,248,255, 255,255,255,255 };

    uint8_t *output = NULL;
    uint8_t *reconstruct = malloc(64);
    size_t size = qoi_encode(test_data, 64, RGBA, &output);
    printf("QOI compressed size: %ld\n", size);
    qoi_decode(output, 26, RGBA, &reconstruct);

    for(size_t i = 0; i < 64; i++)
    {
        printf("%u ", reconstruct[i]);
        if ((i+1)%16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");

    free(output);
    free(reconstruct);
    return 0;
}