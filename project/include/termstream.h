#ifndef PIX2ASCII_TERMSTREAM_H
#define PIX2ASCII_TERMSTREAM_H

#include "frame_processing.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t uS_elapsed;
    size_t frame_index;
    size_t time_frame_index;
    size_t frame_desync;
    size_t cur_frame_processing_time;
} debug_info_t;

void update_terminal_size(frame_params_t *frame_params);

void draw_symbol_frame(frame_params_t *frame_params,
                       const char char_set[],
                       unsigned int max_char_set_index,
                       region_intensity_t get_region_intensity);

void set_color_pairs();

typedef void (*draw_frame_t)(frame_params_t *frame_params,
                             const char char_set[],
                             unsigned int max_char_set_index,
                             region_intensity_t get_region_intensity);

void draw_color_frame(frame_params_t *frame_params,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity);

void debug(const debug_info_t *debug_info, FILE *logs);

#endif //PIX2ASCII_TERMSTREAM_H
