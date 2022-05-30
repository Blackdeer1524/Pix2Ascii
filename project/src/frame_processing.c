#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "frame_processing.h"
#include "status_codes.h"

int update_naive(double **kernel, int width, int height) {
    double *new_kernel = realloc(*kernel, sizeof(double) * width * height);
    if (!new_kernel) {
        fprintf(stderr, "Couldn't update kernel size!");
        return KERNEL_UPDATE_ERROR;
    }

    int kernel_area = width * height;
    for (int row_ind = 0 ; row_ind < height ; ++row_ind)
        for (int col_ind = 0 ; col_ind < width ; ++col_ind)
            new_kernel[row_ind * width + col_ind] = 1.0 / kernel_area;

    *kernel = new_kernel;
    return SUCCESS;
}

int update_gaussian(double **kernel, int width, int height) {
    double *new_kernel = realloc(*kernel, sizeof(double) * width * height);
    if (!new_kernel) {
        fprintf(stderr, "Couldn't update kernel size!");
        return KERNEL_UPDATE_ERROR;
    }
    double sigma = width / 3;
    const double double_sqr_sigma = 2 * sigma * sigma;

    double sum = 0.0;
    int row_ind, col_ind;
    int sqr_x, sqr_y;

    for (row_ind = 0; row_ind < height; ++row_ind)
        for (col_ind = 0; col_ind < width; ++col_ind) {
            sqr_x = col_ind - width / 2;
            sqr_x *= sqr_x;
            sqr_y = row_ind - height / 2;
            sqr_y *= sqr_y;

            new_kernel[row_ind * width + col_ind] = exp(-(sqr_y + sqr_x) / (double_sqr_sigma)) /
                    (M_PI * double_sqr_sigma);
            sum += new_kernel[row_ind * width + col_ind];
    }

    for (row_ind = 0 ; row_ind < height ; ++row_ind)
        for (col_ind = 0 ; col_ind < width ; ++col_ind)
            new_kernel[row_ind * width + col_ind] /= sum;

    *kernel = new_kernel;
    return SUCCESS;
}

void convolve(const frame_params_t *frame_params,
              const kernel_params_t *kernel_params,
              int cur_pixel_row,
              int cur_pixel_col,
              double *r, double *g, double *b) {
    double local_r = 0, local_g = 0, local_b =0;

    int down_row = cur_pixel_row + kernel_params->width;
    int right_col = cur_pixel_col + kernel_params->height;

    int triple_cur_pixel_col = cur_pixel_col * 3;
    int c = 0;
    for (int row_offset = frame_params->triple_width * cur_pixel_row, i = cur_pixel_row;
         i < down_row;
         row_offset += frame_params->triple_width, ++i) {
        for (int col_offset = triple_cur_pixel_col, j = cur_pixel_col;
             j < right_col;
             col_offset += 3, ++j) {
            local_r += kernel_params->kernel[c] * frame_params->video_frame[row_offset + col_offset];
            local_g += kernel_params->kernel[c] * frame_params->video_frame[row_offset + col_offset + 1];
            local_b += kernel_params->kernel[c] * frame_params->video_frame[row_offset + col_offset + 2];
            ++c;
        }
    }
    *r = local_r;
    *g = local_g;
    *b = local_b;
}

unsigned char average_chanel_intensity(double r, double g, double b) {
    return (unsigned char) ((r + g + b) / 3);
}


unsigned char yuv_intensity(double r, double g, double b) {
    return (unsigned char) (r * 0.299 + 0.587 * g + 0.114 * b);
}
