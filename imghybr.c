#include "img.h"
#include "omp.h"
#include <pthread.h>

int num_threads;

typedef struct thread_info {
    pthread_t   thread_id;          /* ID returned by pthread_create() */
    int         thread_num;         /* application-defined thread # */
    int         lw, lh, rw, rh;     /* chunk image left and right corners */
    image       *in_img, *out_img;  /* input and output image */
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

static void
image_posterize(thread_info *tinfo)
{
    int w, h;

    #pragma omp parallel for collapse(2) private(w)
    for (h = tinfo->lh; h < tinfo->rh; h++)
    {
        for (w = tinfo->lw; w < tinfo->rw; w++)
        {
            tinfo->out_img->pix[h][w].red = pixel_reduce(tinfo->in_img->pix[h][w].red);
            tinfo->out_img->pix[h][w].green = pixel_reduce(tinfo->in_img->pix[h][w].green);
            tinfo->out_img->pix[h][w].blue = pixel_reduce(tinfo->in_img->pix[h][w].blue);

        }
    }
}

static void *
thread_posterize (void * arg)
{
    thread_info *tinfo = arg;
    image_posterize(tinfo);
    return NULL;

}

static void
image_pixelate(thread_info *tinfo)
{
    int w, h, pw, ph, avg_red, avg_green, avg_blue, pixel_count;

    avg_red = 0;
    avg_green = 0;
    avg_blue = 0;
    pixel_count = 0;

    #pragma omp parallel for collapse(2) \
     private(w, ph, pw) \
     reduction(+: avg_red, avg_green, avg_blue, pixel_count)
    for (h = tinfo->lh; h < tinfo->rh; h += PIXELATE_RATIO)
    {
        for (w = tinfo->lw; w < tinfo->rw; w+= PIXELATE_RATIO)
        {
            /* new submatrix region - compute value of pixels */
            if (h % PIXELATE_RATIO == 0 && w % PIXELATE_RATIO == 0) {
                avg_red = 0;
                avg_green = 0;
                avg_blue = 0;
                pixel_count = 0;

                for (ph = h; ph < h + PIXELATE_RATIO 
                    && ph < tinfo->out_img->height; ph++) {
                    for (pw = w; pw < w + PIXELATE_RATIO 
                        && pw < tinfo->out_img->width; pw++) {
                        avg_red += tinfo->in_img->pix[ph][pw].red;
                        avg_green += tinfo->in_img->pix[ph][pw].green;
                        avg_blue += tinfo->in_img->pix[ph][pw].blue;
                        pixel_count++;
                    }
                }

                avg_red /= pixel_count;
                avg_green /= pixel_count;
                avg_blue /= pixel_count;
            }

            /* fill in final image pixels */
            for (ph = h; ph < h + PIXELATE_RATIO 
                && ph < tinfo->out_img->height; ph++) {
                for (pw = w; pw < w + PIXELATE_RATIO 
                    && pw < tinfo->out_img->width; pw++) {
                    tinfo->out_img->pix[ph][pw].red = avg_red;
                    tinfo->out_img->pix[ph][pw].green = avg_green;
                    tinfo->out_img->pix[ph][pw].blue = avg_blue;
                }
            }
        }
    }
}

static void *
thread_pixelate (void * arg)
{
    thread_info *tinfo = arg;
    image_pixelate(tinfo);
    return NULL;

}

int main(int argc, char const *argv[])
{
    const char *filter, *filein, *fileout;
    struct timespec start, end;
    int hchunk, wchunk;
    image *img, *out_img;
    long t;
    thread_info *tinfo;
    void *res;

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

    tinfo = calloc(num_threads, sizeof(thread_info));

    /* apply posterize or pixelate filter to image */
    if (strcmp("pixelate", filter) == 0)
    {
        clock_gettime (CLOCK_MONOTONIC, &start);
         out_img = image_new(img->width, img->height);

        /* complete chunk image header */
        strcpy(out_img->type, img->type);
        out_img->width = img->width;
        out_img->height = img->height;
        out_img->maxval = img->maxval;

        /* create num_threads threads */
        for (t = 0; t < num_threads; t++)
        {
            tinfo[t].thread_num = t + 1;

            /* send more pixels to last thread if height or width are not
               perfectly divisable to num_threads */
            if (t + 1 == num_threads) {
                hchunk = img->height;
                wchunk = img->width;

                /* store top left corner and bottom right corner of each chunk */
                if (t > 1) {
                    tinfo[t].lh = tinfo[t - 1].lh;
                    tinfo[t].lw = tinfo[t - 1].lw;
                    tinfo[t].rh = hchunk;
                    tinfo[t].rw = wchunk;
                }

                else {
                    tinfo[t].lh = 0;
                    tinfo[t].lw = 0;
                    tinfo[t].rh = hchunk;
                    tinfo[t].rw = wchunk;
                }
            }

            else {
                hchunk = img->height / num_threads;
                while (hchunk % PIXELATE_RATIO != 0)
                    hchunk++;
                wchunk = img->width;

                /* store top left corner and bottom right corner of each chunk */
                tinfo[t].lh = t * hchunk;
                tinfo[t].lw = 0;
                tinfo[t].rh = (t + 1) * hchunk;
                tinfo[t].rw = wchunk;
            }

            tinfo[t].in_img = img;
            tinfo[t].out_img = out_img;


            /* send chunks to each thread */
            pthread_create(&tinfo[t].thread_id,
                           NULL,
                           &thread_pixelate,
                           &tinfo[t]);
        }


        /* gather processed chunk images from all threads */
        for (t = 0; t < num_threads; t++) {
            pthread_join(tinfo[t].thread_id, &res);
        }

        clock_gettime (CLOCK_MONOTONIC, &end);

        image_write(out_img, fileout);
    }

    else if (strcmp("posterize", filter) == 0)
    {
        clock_gettime (CLOCK_MONOTONIC, &start);

        /* create chunk image for further processing */
        out_img = image_new(img->width, img->height);

        /* complete chunk image header */
        strcpy(out_img->type, img->type);
        out_img->width = img->width;
        out_img->height = img->height;
        out_img->maxval = img->maxval;

        hchunk = img->height / num_threads;
        wchunk = img->width;

        /* create num_threads threads */
        for (t = 0; t < num_threads; t++)
        {
            tinfo[t].thread_num = t + 1;

            /* send more pixels to last thread if height or width are not
               perfectly divisable to num_threads */
            if (t + 1 == num_threads) {
                hchunk = img->height;
                wchunk = img->width;

                /* store top left corner and bottom right corner of each chunk */
                if (t > 1) {
                    tinfo[t].lh = tinfo[t - 1].lh;
                    tinfo[t].lw = tinfo[t - 1].lw;
                    tinfo[t].rh = hchunk;
                    tinfo[t].rw = wchunk;
                }

                else {
                    tinfo[t].lh = 0;
                    tinfo[t].lw = 0;
                    tinfo[t].rh = hchunk;
                    tinfo[t].rw = wchunk;
                }
            }

            else {
                /* store top left corner and bottom right corner of each chunk */
                tinfo[t].lh = t * hchunk;
                tinfo[t].lw = 0;
                tinfo[t].rh = (t + 1) * hchunk;
                tinfo[t].rw = wchunk;
            }

            tinfo[t].in_img = img;
            tinfo[t].out_img = out_img;


            /* send chunks to each thread */
            pthread_create(&tinfo[t].thread_id,
                           NULL,
                           &thread_posterize,
                           &tinfo[t]);
        }


        /* gather processed chunk images from all threads */
        for (t = 0; t < num_threads; t++) {
            pthread_join(tinfo[t].thread_id, &res);
        }

        clock_gettime (CLOCK_MONOTONIC, &end);

        image_write(out_img, fileout);
    }

    else
    {
        printf("Usage: Please choose between \"posterize\" and \"pixelate\" as filter\n");
    }

    /* clean up */
    image_free(img, img->height);
    image_free(out_img, out_img->height);
    free(tinfo);

    /* determine serial time for later comparison */
    printf("Hybrid running time for %d threads is: %f\n", num_threads,
            end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0);

    return 0;
}