#include "img.h"
#include "mpi.h"

int num_tasks;

#define BUFFSIZE       16777216 // 16 MiB
#define TAG_SIZE       43
#define TAG_WORK       42

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

// static void
// image_posterize(thread_info *tinfo)
// {
//     int w, h;

//     /* loop through chunk image and complete final image pixels */
//     for (h = tinfo->lh; h < tinfo->rh; h++)
//     {
//         for (w = tinfo->lw; w < tinfo->rw; w++)
//         {
//             tinfo->out_img->pix[h][w].red = pixel_reduce(tinfo->in_img->pix[h][w].red);
//             tinfo->out_img->pix[h][w].green = pixel_reduce(tinfo->in_img->pix[h][w].green);
//             tinfo->out_img->pix[h][w].blue = pixel_reduce(tinfo->in_img->pix[h][w].blue);

//         }
//     }
// }

// static void
// image_pixelate(thread_info *tinfo)
// {
//     int w, h, pw, ph, avg_red, avg_green, avg_blue, pixel_count;

//     /* loop through chunk image matrix and determine pixel color for submatrixes */
//     for (h = tinfo->lh; h < tinfo->rh; h += PIXELATE_RATIO)
//     {
//         for (w = tinfo->lw; w < tinfo->rw; w+= PIXELATE_RATIO)
//         {
//             /* new submatrix region - compute value of pixels */
//             if (h % PIXELATE_RATIO == 0 && w % PIXELATE_RATIO == 0) {
//                 avg_red = 0;
//                 avg_green = 0;
//                 avg_blue = 0;
//                 pixel_count = 0;

//                 for (ph = h; ph < h + PIXELATE_RATIO 
//                     && ph < tinfo->out_img->height; ph++) {
//                     for (pw = w; pw < w + PIXELATE_RATIO 
//                         && pw < tinfo->out_img->width; pw++) {
//                         avg_red += tinfo->in_img->pix[ph][pw].red;
//                         avg_green += tinfo->in_img->pix[ph][pw].green;
//                         avg_blue += tinfo->in_img->pix[ph][pw].blue;
//                         pixel_count++;
//                     }
//                 }

//                 avg_red /= pixel_count;
//                 avg_green /= pixel_count;
//                 avg_blue /= pixel_count;
//             }

//             /* fill in final image pixels */
//             for (ph = h; ph < h + PIXELATE_RATIO 
//                 && ph < tinfo->out_img->height; ph++) {
//                 for (pw = w; pw < w + PIXELATE_RATIO 
//                     && pw < tinfo->out_img->width; pw++) {
//                     tinfo->out_img->pix[ph][pw].red = avg_red;
//                     tinfo->out_img->pix[ph][pw].green = avg_green;
//                     tinfo->out_img->pix[ph][pw].blue = avg_blue;
//                 }
//             }
//         }
//     }
// }

int main(int argc, char *argv[])
{
    const char *filter, *filein, *fileout;
    // struct timespec start, end; //
    int h, k, rank, worker_id, num_workers, master_id, i, hchunk, wchunk, lh, rh, lw, rw;
    image *img, *out_img;
    void *res;
    MPI_Status status;
    int size_buffer[4];
    RGBpix **buffer_img;

    if (argc < 4) {
        printf("Usage: mpirun -np <num_tasks> <executable> <filter_name> <input_file> <output_file>\n");
        return 0;
    }

    filter = argv[1];
    filein = argv[2];
    fileout = argv[3];

    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    master_id = num_workers = num_tasks - 1;

    buffer_img = rgbpix_new(1000, 1000);
    /* apply posterize or pixelate filter to image */
    if (strcmp("pixelate", filter) == 0)
    {
        int source;
        if (rank == master_id) {
            img = image_read(filein);

            // start = MPI_Wtime();

            // worker_id = 0;
            for (i = 0; i < num_workers; i++) {
                hchunk = img->height / num_workers;
                // while (hchunk % PIXELATE_RATIO != 0)
                //     hchunk++;
                wchunk = img->width;

                /* store top left corner and bottom right corner of each chunk */
                size_buffer[0] = lh = i * hchunk;
                size_buffer[1] = lw = 0;
                size_buffer[2] = rh = (i + 1) * hchunk;
                size_buffer[3] = rw = wchunk;

                // if (i + 1 == num_workers) {
                //     size_buffer[2] = rh = img->height;
                // }

                MPI_Send (size_buffer, 4, MPI_INT, i, TAG_SIZE, MPI_COMM_WORLD);
                printf("[%d] Sent\n", rank);

                // buffer_img = rgbpix_new(rw - lw, rh - lh);
                k = 0;
                for (h = lh; h < rh; h++)
                    memcpy(buffer_img[k++], img->pix[h], img->width * sizeof(RGBpix));

                MPI_Send(buffer_img, (rw - lw) * (rh - lh) * sizeof(RGBpix), MPI_UNSIGNED_CHAR, i, TAG_WORK, MPI_COMM_WORLD);
            }

            for (i = 0; i < num_workers; i++) {
                MPI_Recv (buffer_img, (rw - lw) * (rh - lh) * sizeof(RGBpix), MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &status);
                source = status.MPI_SOURCE;
                printf("[%d] Am primit de la %d\n", rank, source);
                hchunk = img->height / num_workers;
                // while (hchunk % PIXELATE_RATIO != 0)
                //     hchunk++;
                lh = source * hchunk;
                lw = 0;
                rh = (source + 1) * hchunk;
                rw = img->width;
                // if (source + 1 == num_workers) {
                //     rh = img->height;
                // }

                printf("Lh %d rh %d\n", lh, rh);
                k = 0;
                for (h = lh; h < rh; h++)
                    memcpy(img->pix[h], buffer_img[k++], img->width * sizeof(RGBpix));
                printf("%d %d %d\n", img->pix[55][55].red, img->pix[55][55].green, img->pix[55][55].blue);
            }

            for (i = 0; i < num_workers; i++) {
                size_buffer[0] = size_buffer[1] = size_buffer[2] = size_buffer[3] = -1;
                MPI_Send (size_buffer, 4, MPI_INT, i, TAG_SIZE, MPI_COMM_WORLD);
                printf("[%d] Lst sent\n", rank);
            }

        }
        else {
            while (1) {
                MPI_Recv(size_buffer, 4, MPI_INT, master_id, TAG_SIZE, MPI_COMM_WORLD, &status);
                printf("[%d] Received %d %d %d %d\n", rank, size_buffer[0], size_buffer[1], size_buffer[2], size_buffer[3]);

                lh = size_buffer[0];
                lw = size_buffer[1];
                rh = size_buffer[2];
                rw = size_buffer[3];

                if (lh == -1 && rh == -1 && lw == -1 && rw == -1) {
                    printf("[%d] Am primit sa ma culc\n", rank);
                    break;
                }

                // buffer_img = rgbpix_new(rw - lw, rh - lh);
                MPI_Recv(buffer_img, (rw - lw) * (rh - lh) * sizeof(RGBpix), MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &status);
                printf("[%d] Am primit %d\n", rank, (rw - lw) * (rh -lh));

                MPI_Send(buffer_img, (rw - lw) * (rh - lh) * sizeof(RGBpix), MPI_UNSIGNED_CHAR, master_id, TAG_WORK, MPI_COMM_WORLD);
            }
        }

        printf("[%d] Ma culc\n", rank);
        // end = MPI_Wtime();

        image_write(img, fileout);
    }

    else if (strcmp("posterize", filter) == 0)
    {
        // clock_gettime (CLOCK_MONOTONIC, &start);

        // clock_gettime (CLOCK_MONOTONIC, &end);

     //   image_write(out_img, fileout);
    }

    else
    {
        printf("Usage: Please choose between \"posterize\" and \"pixelate\" as filter\n");
    }

    MPI_Finalize ();

    /* clean up */
    // image_free(img, img->height);
    // image_free(out_img, out_img->height);

    /* determine serial time for later comparison */
    // printf("MPI running time for %d threads is: %f\n", num_tasks,
            // end - start);

    return 0;
}
