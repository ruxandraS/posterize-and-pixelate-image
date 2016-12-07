#include "omp.h"
#include "time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIXELATE_RATIO 25

typedef unsigned char pixel;

typedef struct RGBpix
{
    pixel red, blue, green;
} RGBpix;

typedef struct image
{
    char type[3];
	int width, height;
	int maxval;
    struct RGBpix **pix;
} image;