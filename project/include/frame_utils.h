#ifndef PROJECT_INCLUDE_FRAME_UTILS_H_
#define PROJECT_INCLUDE_FRAME_UTILS_H_

#include "utils.h"
#include <ncurses.h>

typedef struct {
    unsigned char *video_frame;
    int width;
    int height;
} frame_data_t;


unsigned char average_chanel_intensity(const unsigned char *video_frame,
                                       unsigned int frame_width,
                                       unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                       unsigned long row_step, unsigned long col_step);

unsigned char yuv_intensity(const unsigned char *video_frame,
                            unsigned int frame_width,
                            unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                            unsigned long row_step, unsigned long col_step);

typedef unsigned char (*region_intensity_t)(const unsigned char *video_frame,
                                            unsigned int frame_width,
                                            unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                            unsigned long row_step, unsigned long col_step);

void draw_frame(const unsigned char *video_frame,
                unsigned int frame_width,
                unsigned int trimmed_height,
                unsigned int trimmed_width,
                unsigned int row_downscale_coef,
                unsigned int col_downscale_coef,
                unsigned int left_border_indent,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity);


#endif  // PROJECT_INCLUDE_FRAME_UTILS_H_
