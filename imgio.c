#include "img.h"

RGBpix ** rgbpix_new(int width, int height)
{
    int h;

    RGBpix **pix = (RGBpix**) calloc(height, sizeof(RGBpix*));
    for (h = 0; h < height; h++)
    {
        pix[h] = (RGBpix*) calloc(width, sizeof(RGBpix));
    }

    return pix;
}

void rgbpix_free(RGBpix **pix, int height)
{
    int h;

    for (h = 0; h < height; h++)
    {
        free(pix[h]);
    }
    free(pix);

}

image * image_new(int width, int height)
{
    image *img = (image*) calloc(1, sizeof(image));
    img->pix = rgbpix_new(width, height);

    return img;
}

void image_free(image *img, int height)
{
    rgbpix_free(img->pix, height);
    free(img);
}

image *image_read(const char *filename)
{
    char type[] = "P6";
    FILE *file;
    int h, w, height, width, maxval;
    image *img;

    /* open file named fileName in read binary mode */
    file = fopen(filename, "rb");

    /* read file header */
    fscanf(file, "%s\n%d %d\n%d\n",
           type, &width, &height, &maxval);
    
    /* alloc memory for image */
    img = image_new(width, height);

    /* complete image header */
    strcpy(img->type, type);
    img->width = width;
    img->height = height;
    img->maxval = maxval;

    /* read image pixels */
    for (h = 0; h < img->height; h++) {
        for (w = 0; w < img->width; w++) {
            fread((void *) &(img->pix[h][w].red), sizeof(pixel), 1, file);
            fread((void *) &(img->pix[h][w].green), sizeof(pixel), 1, file);
            fread((void *) &(img->pix[h][w].blue), sizeof(pixel), 1, file);
        }
    }

    /* clean up */
    fclose(file);

    return img;
}

void image_write(image *img, const char *filename)
{
    FILE *file;
    int h, w;

    /* open file named fileName in write binary mode */
    file = fopen(filename, "wb");

    /* write the header to the file */
    fprintf(file, "%s\n%d% d\n%d\n",
            img->type, img->width, img->height, img->maxval);

    /* write pixels to the file */
    for (h = 0; h < img->height; h++)
    {
        for (w = 0; w < img->width; w++)
        {
            fwrite((void*) &(img->pix[h][w].red), sizeof(pixel), 1, file);
            fwrite((void*) &(img->pix[h][w].green), sizeof(pixel), 1, file);
            fwrite((void*) &(img->pix[h][w].blue), sizeof(pixel), 1, file);
        }
    }

    /* clean up */
    fclose(file);
}
