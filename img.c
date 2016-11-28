#include "img.h"

static void cat(FILE *fp)
{
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), fp) != 0)
         fputs(buffer, stdout);
}

void readPixels(const char *filename, image *img)
{
    int h, w;

    /* open file named fileName in read mode */
    FILE *file;
    file = fopen(filename, "rb");

    /* read file header and check for error */
    fscanf(file, "%s\n%d %d\n%d\n",
            img->type, &img->width, &img->height, &img->maxval);

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
printf("%s\n", img->pix[0][0].red);
    /* clean up */
    fclose(file);
}

void writePixels(const char *filename, image *img)
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

    cat(file);

    /* clean up */
    fclose(file);
}

unsigned char reduce(unsigned char pixel)
{
    if (0 <= (int)pixel && (int)pixel < 64)
        return 0;
    else if (64 <= (int)pixel && (int)pixel < 128)
        return 64;
    else if (128 <= (int)pixel && (int)pixel < 192)
        return 128;
    else if (192 <= (int)pixel && (int)pixel < 256)
        return 192;

    return 0;
}

image* posterize(image *img)
{
    printf("%c\n", img->pix[120][100].red);
    int w, h;

    image *postimg = malloc (sizeof (struct image));
    postimg->pix = (struct RGBpix**) calloc(img->height, sizeof(struct RGBpix*));
    for (h = 0; h < img->height; h++)
    {
        img->pix[h] = (struct RGBpix*) calloc(img->width, sizeof(struct RGBpix));
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
            postimg->pix[w][h].red = reduce(img->pix[w][h].red);
            postimg->pix[w][h].green = reduce(img->pix[w][h].green);
            postimg->pix[w][h].blue = reduce(img->pix[w][h].blue);

        }
    }
    return postimg;
}

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        printf("Usage: <executable> <input_file> <output_file>\n");
    }

    const char *filein = argv[1];
    const char *fileout = argv[2];
    image *img = malloc (sizeof (struct image));

    readPixels (filein, img);
    image *postimg = posterize(img);
    writePixels(fileout, img);
    return 0;
}