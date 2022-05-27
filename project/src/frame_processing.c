#include "frame_processing.h"
#include "utils.h"


void process_block(const frame_params_t *frame_params,
                   const kernel_params_t *kernel_params,
                   int cur_pixel_row,
                   int cur_pixel_col,
                   unsigned int *r, unsigned int *g, unsigned int *b) {
    unsigned int local_r = 0, local_g = 0, local_b =0;

    int down_row = cur_pixel_row + kernel_params->width;
    int right_col = cur_pixel_col + kernel_params->height;

    int triple_cur_pixel_col = cur_pixel_col * 3;
    for (int row_offset = frame_params->triple_width * cur_pixel_row, i = cur_pixel_row;
         i < down_row;
         row_offset += frame_params->triple_width, ++i) {
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

unsigned char average_chanel_intensity(unsigned int r, unsigned int g, unsigned int b,
                                       int normalization) {
    return (r + g + b) / normalization;
}


unsigned char yuv_intensity(unsigned int r, unsigned int g, unsigned int b,
                            int normalization) {
    double current_block_intensity = (r * 0.299 + 0.587 * g + 0.114 * b) / normalization;
    return (unsigned char) MIN(current_block_intensity, 255);
}
