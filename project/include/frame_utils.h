#ifndef PROJECT_INCLUDE_FRAME_UTILS_H_
#define PROJECT_INCLUDE_FRAME_UTILS_H_

#include <ncurses.h>

#include "utils.h"

typedef struct frame_t {
    const unsigned char *video_frame;
    int frame_width;
    int cur_pixel_row;
    int cur_pixel_col;
    int row_step;
    int col_step;
} frame_t;

unsigned char average_chanel_intensity(const unsigned char *video_frame,
                                       int frame_width,
                                       int cur_pixel_row,  int cur_pixel_col,
                                       int row_step, int col_step);

unsigned char yuv_intensity(const unsigned char *video_frame,
                            int frame_width,
                            int cur_pixel_row,  int cur_pixel_col,
                            int row_step, int col_step);

typedef unsigned char (*region_intensity_t)(const unsigned char *video_frame,
                                            int frame_width,
                                            int cur_pixel_row,  int cur_pixel_col,
                                            int row_step, int col_step);

void draw_frame(const unsigned char *video_frame,
                int frame_width,
                int trimmed_height,
                int trimmed_width,
                int row_downscale_coef,
                int col_downscale_coef,
                int left_border_indent,
                const char char_set[],
                int max_char_set_index,
                region_intensity_t get_region_intensity);




#endif  // PROJECT_INCLUDE_FRAME_UTILS_H_
