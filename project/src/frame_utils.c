#include "frame_utils.h"

static char get_char_given_intensity(unsigned char intensity,
                                     const char *char_set,
                                     int max_index) {
    return char_set[max_index - intensity * max_index / 255];
}

static int process_block(const unsigned char *video_frame,
                         int frame_width,
                         int cur_pixel_row,
                         int cur_pixel_col,
                         int row_step,
                         int col_step,
                         int *r, int *g, int *b) {
    static int local_r, local_g, local_b;
    local_r=0, local_g=0, local_b=0;

    static int down_row, right_col;
    down_row = cur_pixel_row + row_step;
    right_col = cur_pixel_col + col_step;

    static int cashed_frame_width;
    static int triple_width;
    if (!cashed_frame_width) {
        triple_width = frame_width * 3;
        cashed_frame_width = frame_width;
    }

    static int triple_cur_pixel_col;
    triple_cur_pixel_col = cur_pixel_col * 3;

    static int row_offset, col_offset;
    static int i, j;
    for (row_offset = triple_width * cur_pixel_row, i = cur_pixel_row;
         i < down_row;
         row_offset += triple_width, ++i) {
        for (col_offset = triple_cur_pixel_col, j = cur_pixel_col;
             j < right_col;
             col_offset += 3, ++j) {
            local_r += video_frame[row_offset + col_offset];
            local_g += video_frame[row_offset + col_offset + 1];
            local_b += video_frame[row_offset + col_offset + 2];
        }
    }
    *r = local_r;
    *g = local_g;
    *b = local_b;
    return 0;
}

unsigned char average_chanel_intensity(const unsigned char *video_frame,
                                       int frame_width,
                                       int cur_pixel_row,  int cur_pixel_col,
                                       int row_step, int col_step) {
    int r, g, b;
    process_block(video_frame, frame_width, cur_pixel_row, cur_pixel_col,
                  row_step,      col_step, &r, &g, &b);
    return (r + g + b) / (row_step * col_step * 3);
}


unsigned char yuv_intensity(const unsigned char *video_frame,
                            int frame_width,
                            int cur_pixel_row,  int cur_pixel_col,
                            int row_step, int col_step) {
    int r, g, b;
    process_block(video_frame, frame_width, cur_pixel_row, cur_pixel_col,
                  row_step,      col_step, &r, &g, &b);
    double current_block_intensity = (r * 0.299 + 0.587 * g + 0.114 * b) / (row_step * col_step * 3);
    return (unsigned char) MY_MIN(current_block_intensity, 255);
}


void draw_frame(const unsigned char *video_frame,
                int frame_width,
                int trimmed_height,
                int trimmed_width,
                int row_downscale_coef,
                int col_downscale_coef,
                int left_border_indent,
                const char char_set[],
                int max_char_set_index,
                region_intensity_t get_region_intensity) {

    for (int cur_char_row=0, cur_pixel_row=0;
         cur_pixel_row < trimmed_height;
         ++cur_char_row, cur_pixel_row += row_downscale_coef) {
        move(cur_char_row, left_border_indent);
        for (int cur_pixel_col=0;
             cur_pixel_col < trimmed_width;
             cur_pixel_col += col_downscale_coef)
            addch(get_char_given_intensity(get_region_intensity(video_frame,
                                                                frame_width,
                                                                cur_pixel_row, cur_pixel_col,
                                                                row_downscale_coef, col_downscale_coef),
                                           char_set, max_char_set_index));
    }
}
