#include "img.h"

static RGBpix **
rgbpix_new(int width, int height)
{
    int h;

    RGBpix **pix = (RGBpix**) calloc(height, sizeof(RGBpix*));
    for (h = 0; h < height; h++)
    {
        pix[h] = (RGBpix*) calloc(width, sizeof(RGBpix));
    }

    return pix;
}

static
void rgbpix_free(RGBpix **pix, int height)
{
    int h;

    for (h = 0; h < height; h++)
    {
        free(pix[h]);
    }
    free(pix);

}

static image * 
image_new(int width, int height)
{
    image *img = (image*) calloc(1, sizeof(image));
    img->pix = rgbpix_new(width, height);

    return img;
}

static
void image_free(image *img, int height)
{
    rgbpix_free(img->pix, height);
    free(img);
}

static image *
image_read(const char *filename)
{
    int h, w, height, width, maxval;
    char type[] = "P6";
    FILE *file;
    image *img;

    /* open file named fileName in read mode */
    file = fopen(filename, "rb");

    /* read file header and check for error */
    fscanf(file, "%s\n%d %d\n%d\n",
           type, &width, &height, &maxval);
    img = image_new(width, height);

    /* complete image header */
    strcpy(img->type, type);
    img->width = width;
    img->height = height;
    img->maxval = maxval;

    /* reading image pixels */
    for (h = 0; h < img->height; h++) {
        for (w = 0; w < img->width; w++) {
            fread((void *) &(img->pix[h][w].red), sizeof(unsigned char), 1, file);
            fread((void *) &(img->pix[h][w].green), sizeof(unsigned char), 1, file);
            fread((void *) &(img->pix[h][w].blue), sizeof(unsigned char), 1, file);
        }
    }

    /* clean up */
    fclose(file);

    return img;
}

static
void image_write(image *img, const char *filename)
{
    int h, w;
    FILE *file;

    /* open file named fileName in read mode */
    file = fopen(filename, "wb");

    /* write the header to the file */
    fprintf(file, "%s\n%d% d\n%d\n", img->type, img->width, img->height, img->maxval);

    /* write pixels to the file */
    for (h = 0; h < img->height; h++)
    {
        for (w = 0; w < img->width; w++)
        {
            fwrite((void*) &(img->pix[h][w].red), sizeof(unsigned char), 1, file);
            fwrite((void*) &(img->pix[h][w].green), sizeof(unsigned char), 1, file);
            fwrite((void*) &(img->pix[h][w].blue), sizeof(unsigned char), 1, file);
        }
    }

    /* clean up */
    fclose(file);
}

static
unsigned char pixel_reduce(unsigned char pixel)
{
    if ((int)pixel < 64)
        return 0;
    else if (64 <= (int)pixel && (int)pixel < 128)
        return 64;
    else if (128 <= (int)pixel && (int)pixel < 192)
        return 128;
    else if (192 <= (int)pixel)
        return 192;

    return 0;
}

static image *
image_posterize(image *img)
{
    int w, h;
    image *posterized;

    posterized = image_new(img->width, img->height);

    /* complete image header */
    strcpy(posterized->type, img->type);
    posterized->width = img->width;
    posterized->height = img->height;
    posterized->maxval = img->maxval;

    for (h = 0; h < posterized->height; h++)
    {
        for (w = 0; w < posterized->width; w++)
        {
            posterized->pix[h][w].red = pixel_reduce(img->pix[h][w].red);
            posterized->pix[h][w].green = pixel_reduce(img->pix[h][w].green);
            posterized->pix[h][w].blue = pixel_reduce(img->pix[h][w].blue);

        }
    }

    return posterized;
}

static image *
image_rotate(image *img, char direction)
{
    int w, h;
    image *posterized;

    posterized = image_new(img->width, img->height);

    /* complete image header */
    strcpy(posterized->type, img->type);
    posterized->width = img->width;
    posterized->height = img->height;
    posterized->maxval = img->maxval;

    for (h = 0; h < posterized->height; h++)
    {
        for (w = 0; w < posterized->width; w++)
        {
            posterized->pix[h][w].red = pixel_reduce(img->pix[h][w].red);
            posterized->pix[h][w].green = pixel_reduce(img->pix[h][w].green);
            posterized->pix[h][w].blue = pixel_reduce(img->pix[h][w].blue);

        }
    }

    return posterized;
}

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        printf("Usage: <executable> <input_file> <output_file>\n");
    }

    const char *filein = argv[1];
    const char *fileout = argv[2];
    image *img;
    image *posterized;

    img = image_read(filein);
    posterized = image_posterize(img);
    image_write(posterized, fileout);

    image_free(img, img->height);
    image_free(posterized, posterized->height);

    return 0;
}