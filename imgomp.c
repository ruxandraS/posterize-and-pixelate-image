#include "img.h"

extern int num_threads;

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

    #pragma omp set_num_of_threads(num_threads)
    #pragma omp parallel
    #pragma omp for private(h, w)
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

static pixel
pixel_pixelate(image *img, char channel, int width, int height)
{
    int h, w, pixel_avg, pixel_count;
    pixel_avg = 0;
    pixel_count = 0;

    for(h = height; h < height + PIXELATE_RATIO && 
        height + PIXELATE_RATIO < img->height; h++) {
        for(w = width; w < width + PIXELATE_RATIO && 
            width + PIXELATE_RATIO < img->width; w++) {
            switch(channel) {
                case 'r':
                    pixel_avg += img->pix[h][w].red;
                    break;
                case 'g':
                    pixel_avg += img->pix[h][w].green;
                    break;
                case 'b':
                    pixel_avg += img->pix[h][w].blue;
                    break;

            }
            
            pixel_count++;
        }
    }

    if (pixel_count)
        return pixel_avg/pixel_count;
    return 0;

}

static image *
image_pixelate(image *img)
{
    int w, h;// pw, ph;
    image *pixelated;

    pixelated = image_new(img->width, img->height);

    /* complete image header */
    strcpy(pixelated->type, img->type);
    pixelated->width = img->width;
    pixelated->height = img->height;
    pixelated->maxval = img->maxval;

    for (h = 0; h < pixelated->height; h++)
    {
        for (w = 0; w < pixelated->width; w++)
        {
            pixelated->pix[h][w].red = pixel_pixelate(img, 'r', w, h);
            pixelated->pix[h][w].green = pixel_pixelate(img, 'g', w, h);
            pixelated->pix[h][w].blue = pixel_pixelate(img, 'b', w, h);

        }
    }

    return pixelated;
}

int main(int argc, char const *argv[])
{
    image *img, *posterized, *pixelated;
    const char *filter, *filein, *fileout;
    double start, end;
    int num_threads;

    if (argc < 5) {
        printf("Usage: <executable> <filter_name> <input_file> <output_file> <num_threads>\n");
        return 0;
    }

    filter = argv[1];
    filein = argv[2];
    fileout = argv[3];
    num_threads = atoi(argv[4]);

    img = image_read(filein);

    if (strcmp("pixelate", filter) == 0)
    {
        start = omp_get_wtime();
        pixelated = image_pixelate(img);
        end = omp_get_wtime();

        image_write(pixelated, fileout);
        image_free(pixelated, pixelated->height);
    }

    else if (strcmp("posterize", filter) == 0)
    {
        start = omp_get_wtime();
        posterized = image_posterize(img);
        end = omp_get_wtime();

        image_write(posterized, fileout);
        image_free(posterized, posterized->height);
    }

    else
    {
        printf("Usage: Please choose between \"posterize\" and \"pixelate\" as filter\n");
    }

    image_free(img, img->height);

    printf("OMP running time is: %f\n", end-start);

    return 0;
}