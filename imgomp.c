#include "img.h"
#include "omp.h"

int num_threads;

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

    #pragma omp parallel for collapse(2) private(w)
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
    int w, h, pw, ph, avg_red, avg_green, avg_blue, pixel_count, hmax, wmax;
    image *pixelated;

    /* alloc memory for image */
    pixelated = image_new(img->width, img->height);

    /* complete image header */
    strcpy(pixelated->type, img->type);
    pixelated->width = img->width;
    pixelated->height = img->height;
    pixelated->maxval = img->maxval;

    avg_red = 0;
    avg_green = 0;
    avg_blue = 0;
    pixel_count = 0;

    #pragma omp parallel for collapse(2) \
     shared(img) private(w, ph, pw, hmax, wmax) \
     reduction(+: avg_red, avg_green, avg_blue, pixel_count)
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

                hmax = min(pixelated->height, h + PIXELATE_RATIO);
                wmax = min(pixelated->width, w + PIXELATE_RATIO);

                for (ph = h; ph < hmax; ph++) {
                    for (pw = w; pw < wmax; pw++) {
                        avg_red += img->pix[ph][pw].red;
                        avg_green += img->pix[ph][pw].green;
                        avg_blue += img->pix[ph][pw].blue;
                        pixel_count++;
                    }
                }

                #pragma omp flush

                avg_red /= pixel_count;
                avg_green /= pixel_count;
                avg_blue /= pixel_count;
            }

            /* fill in image pixels */
            for (ph = h; ph < hmax; ph++) {
                for (pw = w; pw < wmax; pw++) {
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
    int num_threads;
    image *img, *posterized, *pixelated;

    if (argc < 5) {
        printf("Usage: <executable> <filter_name> <input_file> <output_file> <num_threads>\n");
        return 0;
    }

    filter = argv[1];
    filein = argv[2];
    fileout = argv[3];
    num_threads = atoi(argv[4]);

    omp_set_num_threads(num_threads);

    img = image_read(filein);

    /* apply posterize or pixelate filter to image */
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

    /* clean up */
    image_free(img, img->height);

    /* determine serial time for later comparison */
    printf("OMP running time for %d threads is: %f\n", num_threads, end-start);

    return 0;
}
