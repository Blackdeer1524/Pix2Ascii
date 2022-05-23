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

#define COMMAND_BUFFER_SIZE 512
static char command_buffer[COMMAND_BUFFER_SIZE];
#include "timestamps.h"
#include "videostream.h"

void debug(const debug_info_t *debug_info,
           FILE *logs) {
    // =============================================
    // debug info about PREVIOUS frame
    // EL uS    - elapsed time (in microseconds) from the start;
    // EL S     - elapsed time (in seconds) from the start;
    // FI       - current Frame Index;
    // TFI      - current Frame Index measured by elapsed time;
    // uSPF     - micro (u) Seconds Per Frame (Canonical value);
    // Cur uSPF - micro (u) Seconds Per Frame (current);
    // Avg uSPF - micro (u) Seconds Per Frame (Avg);
    // FPS      - Frames Per Second;
    size_t uS_per_frame  = debug_info->uS_elapsed / debug_info->frame_index +
            (debug_info->uS_elapsed % debug_info->frame_index != 0);
    // "EL uS:%10llu|EL S:%8.2f|FI:%5llu|TFI:%5llu|TFI - FI:%2d|uSPF:%8llu|Cur uSPF:%8llu|Avg uSPF:%8llu|FPS:%8f"
    snprintf(command_buffer, COMMAND_BUFFER_SIZE,
             "EL uS:%10zu|EL S:%8.2f|FI:%5zu|TFI:%5zu|abs(TFI - FI):%2zu|uSPF:%8d|Cur uSPF:%8zu|Avg uSPF:%8zu|FPS:%8Lf",
             debug_info->uS_elapsed,
             (double) debug_info->uS_elapsed / N_uSECONDS_IN_ONE_SEC,
             debug_info->frame_index,
             debug_info->time_frame_index,
             debug_info->frame_desync,
             N_uSECONDS_IN_ONE_SEC / VIDEO_FRAMERATE,
             debug_info->cur_frame_processing_time,
             uS_per_frame,
             debug_info->frame_index / ((long double) debug_info->uS_elapsed / N_uSECONDS_IN_ONE_SEC)
             );
    printw("\n%s\n", command_buffer);
    fprintf(logs, "%s\n", command_buffer);
}


void draw_frame(frame_params_t *frame_params,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity) {
    update_terminal_size(frame_params);

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





