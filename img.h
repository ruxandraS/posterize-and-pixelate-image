#include "time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIXELATE_RATIO 10

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })

typedef unsigned char pixel;

typedef struct RGBpix {
    pixel   red, blue, green;
} RGBpix;

typedef struct image {
    char    type[3];
    int     width, height;
    int     maxval;
    struct  RGBpix **pix;
} image;

RGBpix ** rgbpix_new(int width, int height);
void rgbpix_free(RGBpix **pix, int height);
image * image_new(int width, int height);
void image_free(image *img, int height);
image *image_read(const char *filename);
void image_write(image *img, const char *filename);
