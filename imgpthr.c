#include "img.h"
#include <pthread.h>

#define NUM_THREADS 4

typedef struct thread_info {    /* Used as argument to thread_start() */
    pthread_t   thread_id;        /* ID returned by pthread_create() */
    int         thread_num;       /* Application-defined thread # */
    int         lw, lh, rw, rh;
    image       *img;      
} thread_info;

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

static void *
thread_posterize (void * arg)
{
    thread_info *tinfo = arg;
    return image_posterize(tinfo->img);
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
    int h, w, hmax, wmax, ph, pw;
    image *img, *posterized, *pixelated;
    long t;
    thread_info *tinfo;
    void *res;

    if (argc < 4) {
        printf("Usage: <executable> <filter_name> <input_file> <output_file>\n");
        return 0;
    }

    filter = argv[1];
    filein = argv[2];
    fileout = argv[3];

    img = image_read(filein);

    tinfo = calloc(NUM_THREADS, sizeof(thread_info));

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
        /* compute dimension of chunks */
        ph = img->height / NUM_THREADS;
        pw = img->width / NUM_THREADS;

        start = clock();

        /* create NUM_THREADS threads */
        for (t = 0; t < NUM_THREADS; t++)
        {
            tinfo[t].thread_num = t + 1;

            /* send more pixels to last thread if height or width are not
               perfectly divisable to NUM_THREADS */
            if (t + 1 == NUM_THREADS) {
                hmax = img->height;
                wmax = img->width;
            }

            else {
                hmax = (t + 1) * ph;
                wmax = (t + 1) * pw;
            }

            /* store top left corner and bottom right corner of each chunk */
            tinfo[t].lh = hmax - t * ph;
            tinfo[t].lw = wmax - t * pw;
            tinfo[t].rh = hmax;
            tinfo[t].rw = wmax;

            /* create chunk image for further processing */
            tinfo[t].img = image_new(pw, ph);

            /* complete chunk image header */
            strcpy(tinfo[t].img->type, img->type);
            tinfo[t].img->width = wmax - t * pw;
            tinfo[t].img->height = hmax - t * ph;
            tinfo[t].img->maxval = img->maxval;

            /* fill in chunk image pixels */
            for (h = t * ph; h < hmax; h++)
            {
                for (w = t * pw; w < wmax; w++)
                {
                    tinfo[t].img->pix[h][w] = img->pix[h][w];
                }
            }

            /* send chunks to each thread */
            pthread_create(&tinfo[t].thread_id, 
                           NULL,
                           &thread_posterize,
                           &tinfo[t]);
        }

        /* create final image */
        posterized = image_new(img->width, img->height);

        /* complete image header */
        strcpy(pixelated->type, img->type);
        pixelated->width = img->width;
        pixelated->height = img->height;
        pixelated->maxval = img->maxval;

        /* gather processed chunk images from all threads */
        for (t = 0; t < NUM_THREADS; t++) {
            pthread_join(tinfo[t].thread_id, &res);

            ph = 0; /* iterate through final image's height */
            pw = 0; /* iterate through final image's width */

            for (h = tinfo[t].lh; h < tinfo[t].rh; h ++)
            {
                for (w = tinfo[t].lw; w < tinfo[t].rw; w++)
                {
                    pixelated[h][w] = (image*)res.pix[ph][pw];
                    pw++;

                    if (pw == (image*)res.width) {
                        ph++;
                        pw = 0;
                    }
                }
            }
        }

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
    printf("Pthreads running time is: %f\n", (end-start)/CLOCKS_PER_SEC);

    return 0;
}