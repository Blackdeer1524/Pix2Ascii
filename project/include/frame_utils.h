#ifndef PROJECT_INCLUDE_FRAME_UTILS_H_
#define PROJECT_INCLUDE_FRAME_UTILS_H_

#include <ncurses.h>

#include "utils.h"

typedef struct frame_t {
    unsigned char *video_frame;
    int frame_width;
    int cur_pixel_row;
    int cur_pixel_col;
    int row_step;
    int col_step;
} frame_t;

typedef unsigned char (*region_intensity_t)(frame_t frame);

typedef struct frame_full_t {
    unsigned char *video_frame;
    int frame_width;
    int trimmed_height;
    int trimmed_width;
    int row_downscale_coef;
    int col_downscale_coef;
    int left_border_indent;
    int max_char_set_index;
    region_intensity_t get_region_intensity;
    char *char_set;
} frame_full_t;



unsigned char average_chanel_intensity(frame_t frame);
unsigned char yuv_intensity(frame_t frame);
void draw_frame(frame_full_t frame);

#endif  // PROJECT_INCLUDE_FRAME_UTILS_H_
