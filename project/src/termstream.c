//
// Created by blackdeer on 5/21/22.
//

#include "termstream.h"
#include "frame_processing.h"
#include "utils.h"

#include <ncurses.h>


static char get_char_given_intensity(unsigned char intensity,
                                     const char *char_set,
                                     unsigned int max_index) {
    return char_set[max_index - intensity * max_index / 255];
}


void update_terminal_size(frame_params_t *frame_params) {
    // current terminal size in rows and cols
    static int n_available_rows = -1, n_available_cols = -1;
    int new_n_available_rows, new_n_available_cols;

    // video frame downsample coefficients
    getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
    if (n_available_rows != new_n_available_rows || n_available_cols != new_n_available_cols) {
        n_available_rows = new_n_available_rows;
        n_available_cols = new_n_available_cols;

        frame_params->row_downscale_coef = MAX((frame_params->height + n_available_rows) / n_available_rows, 1);
        frame_params->col_downscale_coef = MAX((frame_params->width  + n_available_cols) / n_available_cols, 1);

        frame_params->trimmed_height = frame_params->height - frame_params->height % frame_params->row_downscale_coef;
        frame_params->trimmed_width = frame_params->width - frame_params->width % frame_params->col_downscale_coef;
        frame_params->left_border_indent = (n_available_cols - frame_params->trimmed_width / frame_params->col_downscale_coef) / 2;
        clear();
    }
}


void draw_frame(const frame_params_t *frame_params,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity) {
    for (int cur_char_row=0, cur_pixel_row=0;
         cur_pixel_row < frame_params->trimmed_height;
         ++cur_char_row, cur_pixel_row += frame_params->row_downscale_coef) {
        move(cur_char_row, frame_params->left_border_indent);
        for (int cur_pixel_col=0;
             cur_pixel_col < frame_params->trimmed_width;
             cur_pixel_col += frame_params->col_downscale_coef)
            addch(get_char_given_intensity(
                    get_region_intensity(frame_params, cur_pixel_row, cur_pixel_col), char_set, max_char_set_index));
    }
}





