#include "frame_processing.h"
#include "utils.h"


static void process_block(const frame_params_t *frame_params,
                          int cur_pixel_row,
                          int cur_pixel_col,
                          unsigned int *r, unsigned int *g, unsigned int *b) {
    unsigned int local_r = 0, local_g = 0, local_b =0;

    int down_row = cur_pixel_row + frame_params->row_downscale_coef;
    int right_col = cur_pixel_col + frame_params->col_downscale_coef;

    static unsigned int cashed_frame_width;
    static unsigned int triple_width;
    if (!cashed_frame_width) {
        triple_width = frame_params->width * 3;
        cashed_frame_width = frame_params->width;
    }

    int triple_cur_pixel_col = cur_pixel_col * 3;
    for (int row_offset = triple_width * cur_pixel_row, i = cur_pixel_row;
         i < down_row;
         row_offset += triple_width, ++i) {
        for (int col_offset = triple_cur_pixel_col, j = cur_pixel_col;
             j < right_col;
             col_offset += 3, ++j) {
            local_r += frame_params->video_frame[row_offset + col_offset];
            local_g += frame_params->video_frame[row_offset + col_offset + 1];
            local_b += frame_params->video_frame[row_offset + col_offset + 2];
        }
    }
    *r = local_r;
    *g = local_g;
    *b = local_b;
}

unsigned char average_chanel_intensity(const frame_params_t *frame_params,
                                       int cur_pixel_row,
                                       int cur_pixel_col) {
    unsigned int r, g, b;
    process_block(frame_params, cur_pixel_row, cur_pixel_col, &r, &g, &b);
    return (r + g + b) / (frame_params->row_downscale_coef * frame_params->col_downscale_coef * 3);
}


unsigned char yuv_intensity(const frame_params_t *frame_params,
                            int cur_pixel_row,
                            int cur_pixel_col) {
    unsigned int r, g, b;
    process_block(frame_params, cur_pixel_row, cur_pixel_col, &r, &g, &b);
    double current_block_intensity = (r * 0.299 + 0.587 * g + 0.114 * b) /
            (frame_params->row_downscale_coef * frame_params->col_downscale_coef * 3);
    return (unsigned char) MIN(current_block_intensity, 255);
}
