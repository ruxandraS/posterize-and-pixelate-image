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
            fread((void *) &(img->pix[h][w].red), sizeof(pixel), 1, file);
            fread((void *) &(img->pix[h][w].green), sizeof(pixel), 1, file);
            fread((void *) &(img->pix[h][w].blue), sizeof(pixel), 1, file);
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
            fwrite((void*) &(img->pix[h][w].red), sizeof(pixel), 1, file);
            fwrite((void*) &(img->pix[h][w].green), sizeof(pixel), 1, file);
            fwrite((void*) &(img->pix[h][w].blue), sizeof(pixel), 1, file);
        }
    }

    /* clean up */
    fclose(file);
}

static
pixel pixel_reduce(pixel pixel)
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
image_pixelate(image *img)
{
    int w, h, pw, ph, avg_red, avg_green, avg_blue, pixel_count;
    image *pixelated;

    pixelated = image_new(img->width, img->height);

    /* complete image header */
    strcpy(pixelated->type, img->type);
    pixelated->width = img->width;
    pixelated->height = img->height;
    pixelated->maxval = img->maxval;

    for (h = 0; h < pixelated->height; h += PIXELATE_RATIO)
    {
        for (w = 0; w < pixelated->width; w+= PIXELATE_RATIO)
        {
            if (h % PIXELATE_RATIO == 0 && w % PIXELATE_RATIO == 0) {
                avg_red = 0;
                avg_green = 0;
                avg_blue = 0;
                pixel_count = 0;
                for (ph = h; ph < h + PIXELATE_RATIO 
                    && ph < pixelated->height; ph++) {
                    for (pw = w; pw < w + PIXELATE_RATIO 
                        && pw < pixelated->width; pw++) {
                        avg_red += img->pix[ph][pw].red;
                        avg_green += img->pix[ph][pw].green;
                        avg_blue += img->pix[ph][pw].blue;
                        pixel_count++;
                    }
                }

                avg_red /= pixel_count;
                avg_green /= pixel_count;
                avg_blue /= pixel_count;
            }

            for (ph = h; ph < h + PIXELATE_RATIO 
                && ph < pixelated->height; ph++) {
                for (pw = w; pw < w + PIXELATE_RATIO 
                    && pw < pixelated->width; pw++) {
                    pixelated->pix[ph][pw].red = avg_red;
                    pixelated->pix[ph][pw].green = avg_green;
                    pixelated->pix[ph][pw].blue = avg_blue;
                }
            }                    
        }
    }

    return pixelated;
}

int main(int argc, char const *argv[])
{
    image *img, *posterized, *pixelated;
    const char *filter, *filein, *fileout;
    double start, end;

    if (argc < 4) {
        printf("Usage: <executable> <filter_name> <input_file> <output_file>\n");
        return 0;
    }

    filter = argv[1];
    filein = argv[2];
    fileout = argv[3];

    img = image_read(filein);

    if (strcmp("pixelate", filter) == 0)
    {
        start = clock();
        pixelated = image_pixelate(img);
        end = clock();

        image_write(pixelated, fileout);
        image_free(pixelated, pixelated->height);
    }

    else if (strcmp("posterize", filter) == 0)
    {
        start = clock();
        posterized = image_posterize(img);
        end = clock();

        image_write(posterized, fileout);
        image_free(posterized, posterized->height);
    }

    else
    {
        printf("Usage: Please choose between \"posterize\" and \"pixelate\" as filter\n");
    }

    image_free(img, img->height);

    printf("Serial running time is: %f\n", (end-start)/CLOCKS_PER_SEC);

    return 0;
}