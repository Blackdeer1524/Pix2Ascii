#ifndef PROJECT_INCLUDE_FRAME_UTILS_H_
#define PROJECT_INCLUDE_FRAME_UTILS_H_

#include "termstream.h"


typedef struct {
    unsigned char *video_frame;
    int width;
    int height;
    int trimmed_width;
    int trimmed_height;
    int row_downscale_coef;
    int col_downscale_coef;
    int left_border_indent;
} frame_params_t;

typedef unsigned char (*region_intensity_t)(const frame_params_t *frame_params,
                                            int cur_pixel_row,
                                            int cur_pixel_col);

unsigned char average_chanel_intensity(const frame_params_t *video_frame,
                                       int cur_pixel_row,
                                       int cur_pixel_col);

unsigned char yuv_intensity(const frame_params_t *frame_params,
                            int cur_pixel_row,
                            int cur_pixel_col);

#endif  // PROJECT_INCLUDE_FRAME_UTILS_H_
