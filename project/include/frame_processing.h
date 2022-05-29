#ifndef PROJECT_INCLUDE_FRAME_UTILS_H_
#define PROJECT_INCLUDE_FRAME_UTILS_H_

typedef struct {
    unsigned char *video_frame;
    int width;
    int height;
    int aspect_ratio;
    int trimmed_width;  // cached for draw_frame
    int trimmed_height;  // cached for draw_frame
    int triple_width;  // cached for convolve
} frame_params_t;

typedef int (*kernel_update_method)(double **kernel, int width, int height);

int update_naive(double **kernel, int width, int height);

int update_gaussian(double **kernel, int width, int height);

typedef struct {
    double *kernel;
    int width;
    int height;
    int volume;  // cached for region_intensity_t: width * height * 3
    kernel_update_method update_kernel;
} kernel_params_t;

void convolve(const frame_params_t *frame_params,
              const kernel_params_t *kernel_params,
              int cur_pixel_row,
              int cur_pixel_col,
              double *r, double *g, double *b);

typedef unsigned char (*region_intensity_t)(double r, double g, double b);

unsigned char average_chanel_intensity(double r, double g, double b);

unsigned char yuv_intensity(double r, double g, double b);

#endif  // PROJECT_INCLUDE_FRAME_UTILS_H_
