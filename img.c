#include "img.h"

static
RGBpix** RGBpix_new(int width, int height)
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
void RGBpix_free(RGBpix** pix, int height)
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
    img->pix = RGBpix_new(width, height);
}

static
void image_free(image* img, int height)
{
    RGBpix_free(img->pix, height);
    free(img);
}

static
void image_read(const char *filename, image *img)
{
    int h, w, height, width, maxval;
    char type[] = "P6";
    FILE *file;
    /* open file named fileName in read mode */
    file = fopen(filename, "rb");

    /* read file header and check for error */
    fscanf(file, "%s\n%d %d\n%d\n",
           type, width, height, maxval);

    img->pix = (struct RGBpix**) calloc(img->height, sizeof(struct RGBpix*));
    for (h = 0; h < img->height; h++)
    {
        img->pix[h] = (struct RGBpix*) calloc(img->width, sizeof(struct RGBpix));
    }

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
}

static
void image_write(const char *filename, image *img)
{
    int h, w;

    /* open file named fileName in read mode & check for error */
    FILE *file;
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

static
void image_posterize(image *img, image *postimg)
{
    //printf("%c\n", img->pix[120][100].red);
    int w, h;

    postimg->pix = (struct RGBpix**) calloc(img->height, sizeof(struct RGBpix*));
    for (h = 0; h < img->height; h++)
    {
        postimg->pix[h] = (struct RGBpix*) calloc(img->width, sizeof(struct RGBpix));
    }

    /* complete image header */
    strcpy(postimg->type, img->type);
    postimg->width = img->width;
    postimg->height = img->height;
    postimg->maxval = img->maxval;

    //printf("%s\n%d% d\n%d\n", postimg->type, postimg->width, postimg->height, postimg->maxval);
    for (h = 0; h < postimg->height; h++)
    {
        for (w = 0; w < postimg->width; w++)
        {
            postimg->pix[h][w].red = pixel_reduce(img->pix[h][w].red);
            postimg->pix[h][w].green = pixel_reduce(img->pix[h][w].green);
            postimg->pix[h][w].blue = pixel_reduce(img->pix[h][w].blue);

        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        printf("Usage: <executable> <input_file> <output_file>\n");
    }

    const char *filein = argv[1];
    const char *fileout = argv[2];
    image *img, *posterized;

    image_read(filein, img);
    image_posterize(img, posterized);
    image_write(fileout, img);
    return 0;
}