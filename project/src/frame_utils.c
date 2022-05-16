#include "frame_utils.h"

static char get_char_given_intensity(unsigned char intensity, const char *char_set, int max_index) {
    return char_set[max_index - intensity * max_index / 255];
}

static int process_block(frame_t frame, int *r, int *g, int *b) {
    int local_r = 0, local_g = 0, local_b = 0;
    int down_row = frame.cur_pixel_row + frame.row_step;
    int right_col = frame.cur_pixel_col + frame.col_step;
    int cashed_frame_width = 0;
    int triple_width;

    if (!cashed_frame_width) {
        triple_width = frame.frame_width * 3;
        cashed_frame_width = frame.frame_width;
    }

    int triple_cur_pixel_col = frame.cur_pixel_col * 3;

    int row_offset, col_offset;
    int i, j;
    for (row_offset = triple_width * frame.cur_pixel_row, i = frame.cur_pixel_row;
         i < down_row;
         row_offset += triple_width, ++i) {
        for (col_offset = triple_cur_pixel_col, j = frame.cur_pixel_col;
             j < right_col;
             col_offset += 3, ++j) {
            local_r += frame.video_frame[row_offset + col_offset];
            local_g += frame.video_frame[row_offset + col_offset + 1];
            local_b += frame.video_frame[row_offset + col_offset + 2];
        }
    }
    *r = local_r;
    *g = local_g;
    *b = local_b;
    return 0;
}

unsigned char average_chanel_intensity(frame_t frame) {
    int r, g, b;

    process_block(frame, &r, &g, &b);

    return (r + g + b) / (frame.row_step * frame.col_step * 3);
}

unsigned char yuv_intensity(frame_t frame) {
    int r, g, b;

    process_block(frame, &r, &g, &b);
    double current_block_intensity = (r * 0.299 + 0.587 * g + 0.114 * b) / (frame.row_step * frame.col_step * 3);

    return (unsigned char) MY_MIN(current_block_intensity, 255);
}


void draw_frame(frame_full_t frame_full) {

    for (int cur_char_row=0, cur_pixel_row=0;
         cur_pixel_row < frame_full.trimmed_height;
         ++cur_char_row, cur_pixel_row += frame_full.row_downscale_coef) {
        move(cur_char_row, frame_full.left_border_indent);
        for (int cur_pixel_col=0; cur_pixel_col < frame_full.trimmed_width; cur_pixel_col += frame_full.col_downscale_coef) {
            frame_t frame = {frame_full.video_frame, frame_full.frame_width, cur_pixel_row, cur_pixel_col, frame_full.row_downscale_coef, frame_full.col_downscale_coef};
            addch(get_char_given_intensity(get_region_intensity(frame), frame_full.char_set, frame_full.max_char_set_index));
        }
    }
}
