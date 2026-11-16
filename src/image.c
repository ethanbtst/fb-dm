// Helpers to for image processing (PPM only right now)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "image.h"

bool create_buffer(buffer *fb, const char *device)
{
    if (fb == NULL || device == NULL) 
        return false;
    
    *fb = (buffer){.fd = -1};
    fb->fd = open(device, O_RDWR);
    if (fb->fd == -1) {
        perror("Failed to open framebuffer device");
        return false;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Failed to get variable screen info");
        close(fb->fd);
        fb->fd = -1;
        return false;
    }

    struct fb_fix_screeninfo finfo;
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Failed to get fixed screen info");
        close(fb->fd);
        fb->fd = -1;
        return false;
    }

    fb->w = (int)vinfo.xres;
    fb->h = (int)vinfo.yres;
    fb->bpp = (int)vinfo.bits_per_pixel;
    fb->line_length = (int)finfo.line_length;
    fb->screensize = fb->line_length * fb->h;
    fb->fbp = mmap(NULL, fb->screensize, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fb->fd, 0);

    if (fb->fbp == MAP_FAILED) {
        perror("Failed to mmap framebuffer device");
        fb->fbp = NULL;
        close(fb->fd);
        fb->fd = -1;
        return false;
    }

    return true;
}

void destroy_buffer(buffer *fb)
{
    if (fb == NULL)
        return;
    if (fb->fbp != NULL)
        munmap(fb->fbp, fb->screensize);
    if (fb->fd != -1)
        close(fb->fd);

    *fb = (buffer){.fd = -1};
    fb->fd = -1;
}

bool load_ppm(image *image, const char *filename)
{
    FILE *fptr = fopen(filename, "rb");

    if (!fptr) {
        perror("Failed to open PPM");
        return false;
    }

    char type[5];
    fscanf(fptr, "%s", type);
    fscanf(fptr, "%d %d %*d", &(image->w), &(image->h));
    pixel *data = malloc(image->w * image->h * sizeof(pixel));

    if (strcmp(type, "P6") == 0) {
        fgetc(fptr);
        fread(data, sizeof(pixel), image->w * image->h, fptr);
    } else if (strcmp(type, "P3") == 0) {
        for (int y = 0; y < image->h; y++) {
            for (int x = 0; x < image->w; x++) {
                int r, g, b;

                if (fscanf(fptr, "%d %d %d", &r, &g, &b) != 3) {
                    fclose(fptr);
                    free(data);
                    perror("Bad PPM File: Pixels do not fit dimensions");
                    return false;
                }

                data[y * image->w + x].r = (uint8_t)r;
                data[y * image->w + x].g = (uint8_t)g;
                data[y * image->w + x].b = (uint8_t)b;
            }
        }
    } else {
        perror("Bad PPM file: Invalid PPM type");
        exit(1);
    }

    image->data = data;
    fclose(fptr);
    return true;
}

void draw_image(buffer *fb, const image *img, int x_offset, int y_offset)
{
    if (fb == NULL || img == NULL || fb->fbp == NULL || img->data == NULL || fb->bpp < 24)
        return;


    int bytes_per_pixel = fb->bpp / 8;
    for (int sy = 0; sy < img->h; sy++) {
        int dy = sy + y_offset;
        if (dy < 0 || dy >= fb->h) 
            continue;

        for (int sx = 0; sx < img->w; sx++) {
            int dx = sx + x_offset;
            if (dx < 0 || dx >= fb->w)
                continue;
           
            size_t location = (size_t)dy * fb->line_length +
                              (size_t)dx * bytes_per_pixel;

            pixel pixel = img->data[sy * img->w + sx];

            fb->fbp[location]     = (char)pixel.b;
            fb->fbp[location + 1] = (char)pixel.g;
            fb->fbp[location + 2] = (char)pixel.r;

            if (bytes_per_pixel == 4) 
                fb->fbp[location + 3] = (char)255;
        }
    }
}