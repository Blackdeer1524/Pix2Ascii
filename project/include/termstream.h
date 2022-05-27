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
} sync_info_t;

void update_terminal_size(frame_params_t *frame_params,
                          kernel_params_t *kernel_params,
                          int *left_border_indent);

void draw_symbol_frame(frame_params_t *frame_params,
                       const char char_set[],
                       unsigned int max_char_set_index,
                       region_intensity_t get_region_intensity);

void set_color_pairs();

typedef void (*display_symbol_t)(char symbol,
                                 const kernel_params_t *kernel_params,
                                 unsigned int r, unsigned int g, unsigned int b);

void simple_display(char symbol,
                    const kernel_params_t *kernel_params,
                    unsigned int r, unsigned int g, unsigned int b);

void colored_display(char symbol,
                    const kernel_params_t *kernel_params,
                    unsigned int r, unsigned int g, unsigned int b);



void draw_frame(const frame_params_t *frame_params,
                const kernel_params_t *kernel_params,
                int left_border_indent,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity,
                display_symbol_t display_symbol);

void debug(const sync_info_t *debug_info, FILE *logs);

#endif //PIX2ASCII_TERMSTREAM_H
