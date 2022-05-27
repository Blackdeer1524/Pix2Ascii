//
// Created by blackdeer on 5/21/22.
//

#include "termstream.h"
#include "frame_processing.h"
#include "utils.h"

#include <ncurses.h>

#define RED_DEPTH 6
#define GREEN_DEPTH 7
#define BLUE_DEPTH 6
#define RED_MULTIPLIER 42   // RED_DEPTH * GREEN_DEPTH
#define GREEN_MULTIPLIER 6  // GREEN_DEPTH
#define BLUE_MULTIPLIER 1
#define COLOR_6_UNITS_MULTIPLIER (1000 / (6 - 1))
#define COLOR_7_UNITS_MULTIPLIER (1000 / (7 - 1))
#define DEPTH_6_CONVERT_DIV 51  // = 255/(6-1)
#define DEPTH_7_CONVERT_DIV 42  // = 255/(7-1)
#define WHITE_COLOR 252         // RED_DEPTH * GREEN_DEPTH * BLUE_DEPTH
#define BLACK_COLOR 1           //
#define TEXT_COLOR_PAIR 253     // Color pair index for text

static char get_char_given_intensity(unsigned char intensity,
                                     const char *char_set,
                                     unsigned int max_index) {
    return char_set[max_index - intensity * max_index / 255];
}

void update_terminal_size(frame_params_t *frame_params,
                          kernel_params_t *kernel_params,
                          int *left_border_indent) {
    // current terminal size in rows and cols
    static int n_available_rows = -1, n_available_cols = -1;
    int new_n_available_rows, new_n_available_cols;

    // video frame downsample coefficients
    getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
    if (n_available_rows != new_n_available_rows || n_available_cols != new_n_available_cols) {
        n_available_rows = new_n_available_rows;
        n_available_cols = new_n_available_cols;

        kernel_params->width = MAX((frame_params->height + n_available_rows) / n_available_rows, 1);
        kernel_params->height = MAX((frame_params->width + n_available_cols) / n_available_cols, 1);
        kernel_params->volume = kernel_params->width * kernel_params->height * 3;
        fill_gaussian_filter(&kernel_params->kernel, kernel_params->width, kernel_params->height);

        frame_params->trimmed_height = frame_params->height - frame_params->height % kernel_params->width;
        frame_params->trimmed_width = frame_params->width - frame_params->width % kernel_params->height;
        *left_border_indent = (n_available_cols - frame_params->trimmed_width / kernel_params->height) / 2;
        clear();
    }
}

#define COMMAND_BUFFER_SIZE 512
static char command_buffer[COMMAND_BUFFER_SIZE];
#include "timestamps.h"
#include "videostream.h"

void debug(const sync_info_t *debug_info,
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

void set_color_pairs() {
    for (int r = 0; r < RED_DEPTH; r++) {
        for (int g = 0; g < GREEN_DEPTH; g++) {
            for (int b = 0; b < BLUE_DEPTH; b++) {
                init_color(r * RED_MULTIPLIER + g * GREEN_MULTIPLIER + b * BLUE_MULTIPLIER + 1,
                           r * COLOR_6_UNITS_MULTIPLIER, g * COLOR_7_UNITS_MULTIPLIER, b * COLOR_6_UNITS_MULTIPLIER);
                init_pair(r * RED_MULTIPLIER + g * GREEN_MULTIPLIER + b * BLUE_MULTIPLIER + 1,
                          WHITE_COLOR - (r * RED_MULTIPLIER + g * GREEN_MULTIPLIER + b * BLUE_MULTIPLIER + 1),
                          r * RED_MULTIPLIER + g * GREEN_MULTIPLIER + b * BLUE_MULTIPLIER + 1);
            }
        }
    }
    init_pair(TEXT_COLOR_PAIR, WHITE_COLOR, BLACK_COLOR);
}

static int get_color_index(unsigned int r, unsigned int g, unsigned int b) {
    // Keep multipliers in origin order!
    return r / DEPTH_6_CONVERT_DIV * RED_MULTIPLIER +
    g / DEPTH_7_CONVERT_DIV * GREEN_MULTIPLIER +
    b / DEPTH_6_CONVERT_DIV + 1;
}

void simple_display(char symbol,
                    const kernel_params_t *kernel_params,
                    unsigned int r, unsigned int g, unsigned int b) {
    addch(symbol);
}

void colored_display(char symbol, unsigned int r, unsigned int g, unsigned int b) {
    int pair = COLOR_PAIR(get_color_index(r, g, b));
    attron(pair);
    addch(symbol);
}

void draw_frame(const frame_params_t *frame_params,
                const kernel_params_t *kernel_params,
                int left_border_indent,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity,
                display_symbol_t display_symbol) {
    double r, g, b;
    char displaying_symbol;

    for (int cur_char_row=0, cur_pixel_row=0;
         cur_pixel_row < frame_params->trimmed_height;
         ++cur_char_row, cur_pixel_row += kernel_params->width) {
        move(cur_char_row, left_border_indent);
        for (int cur_pixel_col=0;
             cur_pixel_col < frame_params->trimmed_width;
             cur_pixel_col += kernel_params->height) {
            convolve(frame_params, kernel_params, cur_pixel_row, cur_pixel_col, &r, &g, &b);
            displaying_symbol = get_char_given_intensity(get_region_intensity(r, g, b), char_set, max_char_set_index);
            display_symbol(displaying_symbol, (unsigned int) r, (unsigned int) g, (unsigned int) b);
        }
    }
}
