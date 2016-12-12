#include "img.h"

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

    /* alloc memory for image */
    posterized = image_new(img->width, img->height);

    /* complete image header */
    strcpy(posterized->type, img->type);
    posterized->width = img->width;
    posterized->height = img->height;
    posterized->maxval = img->maxval;

    /* complete image pixels */
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

    /* alloc memory for image */
    pixelated = image_new(img->width, img->height);

    /* complete image header */
    strcpy(pixelated->type, img->type);
    pixelated->width = img->width;
    pixelated->height = img->height;
    pixelated->maxval = img->maxval;

    /* loop through image matrix and determine pixel color for submatrixes */
    for (h = 0; h < pixelated->height; h += PIXELATE_RATIO)
    {
        for (w = 0; w < pixelated->width; w+= PIXELATE_RATIO)
        {
            /* new submatrix region - compute value of pixels */
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

            /* fill in image pixels */
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
    const char *filter, *filein, *fileout;
    double start, end;
    image *img, *posterized, *pixelated;

    if (argc < 4) {
        printf("Usage: <executable> <filter_name> <input_file> <output_file>\n");
        return 0;
    }

    filter = argv[1];
    filein = argv[2];
    fileout = argv[3];

    img = image_read(filein);

    /* apply posterize or pixelate filter to image */
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

    /* clean up */
    image_free(img, img->height);

    /* determine serial time for later comparison */
    printf("Serial running time is: %f\n", (end-start)/CLOCKS_PER_SEC);

    return 0;
}
