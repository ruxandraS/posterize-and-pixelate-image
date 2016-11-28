//#include "omp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct RGBpix
{
    unsigned char red, blue, green;
} RGBpix;

typedef struct image
{
    char type[3];
	int width, height;
	int maxval;
    struct RGBpix **pix;
} image;