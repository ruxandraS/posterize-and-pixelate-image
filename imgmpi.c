#include "img.h"
#include "mpi.h"

int num_tasks;

#define BUFFSIZE       16777216 // 16 MiB
#define TAG_SIZE       43
#define TAG_WORK       42


RGBpix **alloc_2d_rgb(int height, int width)
{
    RGBpix *data = (RGBpix *)malloc(height*width*sizeof(RGBpix));
    RGBpix **array= (RGBpix **)malloc(height*sizeof(RGBpix*));
    for (int i=0; i<height; i++)
        array[i] = &(data[width*i]);

    return array;
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

static RGBpix**
image_posterize(RGBpix **pix, int width, int height)
{
    int w, h;
    RGBpix **posterized;

    // posterized = rgbpix_new(width, height);
    posterized = alloc_2d_rgb(height, width);
    /* loop through chunk image and complete final image pixels */
    for (h = 0; h < height; h++)
    {
        for (w = 0; w < width; w++)
        {
            posterized[h][w].red = pixel_reduce(pix[h][w].red);
            posterized[h][w].green = pixel_reduce(pix[h][w].green);
            posterized[h][w].blue = pixel_reduce(pix[h][w].blue);

        }
    }

    return posterized;
}

static RGBpix **
image_pixelate(RGBpix **pix, int width, int height)
{
    int w, h, pw, ph, avg_red, avg_green, avg_blue, pixel_count;
    image *pixelated;

    /* alloc memory for image */
    pixelated = image_new (width, height);
    rgbpix_free(pixelated->pix, height);
    pixelated->pix = alloc_2d_rgb(height, width);

    /* complete image header */
    strcpy(pixelated->type, "P6");
    pixelated->width = width;
    pixelated->height = height;
    pixelated->maxval = 255;

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
                        avg_red += pix[ph][pw].red;
                        avg_green += pix[ph][pw].green;
                        avg_blue += pix[ph][pw].blue;
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

    return pixelated->pix;
}

int main(int argc, char *argv[])
{
    const char *filter, *filein, *fileout;
    int h, k, rank, num_workers, master_id, i;
    image *img, *out_img;
    MPI_Status status;
    int size_buffer[2];
    RGBpix **buffer_img;
    int chunk_size;
    double start, end;

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

    int blocksCount = 3;
    int blocksLength[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
    MPI_Aint offsets[3];
    MPI_Datatype RGB_TYPE;
    offsets[0] = offsetof(RGBpix, red);
    offsets[1] = offsetof(RGBpix, green);
    offsets[2] = offsetof(RGBpix, blue);

    MPI_Type_create_struct(blocksCount, blocksLength, offsets, types, &RGB_TYPE);
    MPI_Type_commit(&RGB_TYPE);


    master_id = num_workers = num_tasks - 1;

    /* apply posterize or pixelate filter to image */
    if (strcmp("pixelate", filter) == 0)
    {
        int source;
        if (rank == master_id) {
            img = image_read(filein);

            start = MPI_Wtime();

            out_img = image_new(img->width, img->height);
            strcpy(out_img->type, "P6");
            out_img->maxval = 255;
            out_img->width = img->width;
            out_img->height = img->height;

            for (i = 0; i < num_workers; i++) {
                /* store top left corner and bottom right corner of each chunk */
                size_buffer[0] = img->width;
                size_buffer[1] = img->height / num_workers;
                chunk_size = size_buffer[0] * size_buffer[1];

                /* Send width and height */
                MPI_Send (size_buffer, 2, MPI_INT, i, TAG_SIZE, MPI_COMM_WORLD);

                /* Copy data into buffer */
                buffer_img = alloc_2d_rgb(size_buffer[1], size_buffer[0]);
                k = 0;
                for (h = i * size_buffer[1]; h < (i + 1) * size_buffer[1]; h++)
                    memcpy(buffer_img[k++], img->pix[h], img->width * sizeof(RGBpix));

                /* Send buffer */
                MPI_Send(&(buffer_img[0][0]), chunk_size, RGB_TYPE, i, TAG_WORK, MPI_COMM_WORLD);
            }

            for (i = 0; i < num_workers; i++) {
                /* Receive buffer from workers */
                MPI_Recv (&(buffer_img[0][0]), chunk_size, RGB_TYPE, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &status);
                source = status.MPI_SOURCE;

                /* Copy data into output image */
                k = 0;
                for (h = source * size_buffer[1]; h < (source + 1) * size_buffer[1]; h++)
                    memcpy(out_img->pix[h], buffer_img[k++], img->width * sizeof(RGBpix));
            }

            /* Send stop to workers */
            for (i = 0; i < num_workers; i++) {
                size_buffer[0] = size_buffer[1] = -1;
                MPI_Send (size_buffer, 2, MPI_INT, i, TAG_SIZE, MPI_COMM_WORLD);
            }

            end = MPI_Wtime();
            printf("MPI running time is: %lf\n", end-start);
            image_write(out_img, fileout);
        }
        else {
            while (1) {
                /* Receive width and height from master */
                MPI_Recv(size_buffer, 2, MPI_INT, master_id, TAG_SIZE, MPI_COMM_WORLD, &status);

                if (size_buffer[0] == -1 && size_buffer[1] == -1) {
                    break;
                }

                chunk_size = size_buffer[0] * size_buffer[1];

                buffer_img = alloc_2d_rgb(size_buffer[1], size_buffer[0]);
                /* Receive data from master */
                MPI_Recv(&(buffer_img[0][0]), chunk_size, RGB_TYPE, master_id, TAG_WORK, MPI_COMM_WORLD, &status);
                printf("[%d] Am primit %d\n", rank, chunk_size);

                buffer_img = image_pixelate(buffer_img, size_buffer[0], size_buffer[1]);

                /* Send data to master */
                MPI_Send(&(buffer_img[0][0]), chunk_size, RGB_TYPE, master_id, TAG_WORK, MPI_COMM_WORLD);
            }
        }
    }

    else if (strcmp("posterize", filter) == 0)
    {
        int source;
        if (rank == master_id) {
            img = image_read(filein);

            start = MPI_Wtime();

            out_img = image_new(img->width, img->height);
            strcpy(out_img->type, "P6");
            out_img->maxval = 255;
            out_img->width = img->width;
            out_img->height = img->height;

            for (i = 0; i < num_workers; i++) {
                /* store top left corner and bottom right corner of each chunk */
                size_buffer[0] = img->width;
                size_buffer[1] = img->height / num_workers;
                chunk_size = size_buffer[0] * size_buffer[1];

                /* Send width and height */
                MPI_Send (size_buffer, 2, MPI_INT, i, TAG_SIZE, MPI_COMM_WORLD);

                /* Copy data into buffer */
                buffer_img = alloc_2d_rgb(size_buffer[1], size_buffer[0]);
                k = 0;
                for (h = i * size_buffer[1]; h < (i + 1) * size_buffer[1]; h++)
                    memcpy(buffer_img[k++], img->pix[h], img->width * sizeof(RGBpix));

                /* Send buffer */
                MPI_Send(&(buffer_img[0][0]), chunk_size, RGB_TYPE, i, TAG_WORK, MPI_COMM_WORLD);
            }

            for (i = 0; i < num_workers; i++) {
                /* Receive buffer from workers */
                MPI_Recv (&(buffer_img[0][0]), chunk_size, RGB_TYPE, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &status);
                source = status.MPI_SOURCE;

                /* Copy data into output image */
                k = 0;
                for (h = source * size_buffer[1]; h < (source + 1) * size_buffer[1]; h++)
                    memcpy(out_img->pix[h], buffer_img[k++], img->width * sizeof(RGBpix));
            }

            /* Send stop to workers */
            for (i = 0; i < num_workers; i++) {
                size_buffer[0] = size_buffer[1] = -1;
                MPI_Send (size_buffer, 2, MPI_INT, i, TAG_SIZE, MPI_COMM_WORLD);
            }

            end = MPI_Wtime();

            image_write(out_img, fileout);
            printf("MPI running time is: %lf\n", end-start);
        }
        else {
            while (1) {
                /* Receive width and height from master */
                MPI_Recv(size_buffer, 2, MPI_INT, master_id, TAG_SIZE, MPI_COMM_WORLD, &status);

                if (size_buffer[0] == -1 && size_buffer[1] == -1) {
                    break;
                }

                chunk_size = size_buffer[0] * size_buffer[1];

                buffer_img = alloc_2d_rgb(size_buffer[1], size_buffer[0]);
                /* Receive data from master */
                MPI_Recv(&(buffer_img[0][0]), chunk_size, RGB_TYPE, master_id, TAG_WORK, MPI_COMM_WORLD, &status);
                printf("[%d] Am primit %d\n", rank, chunk_size);

                buffer_img = image_posterize(buffer_img, size_buffer[0], size_buffer[1]);

                /* Send data to master */
                MPI_Send(&(buffer_img[0][0]), chunk_size, RGB_TYPE, master_id, TAG_WORK, MPI_COMM_WORLD);
            }
        }
    }

    else
    {
        printf("Usage: Please choose between \"posterize\" and \"pixelate\" as filter\n");
    }

    MPI_Finalize ();

    /* clean up */
    // image_free(img, img->height);
    // image_free(out_img, out_img->height);

    return 0;
}
