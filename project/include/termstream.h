#ifndef PIX2ASCII_TERMSTREAM_H
#define PIX2ASCII_TERMSTREAM_H

#include "frame_processing.h"

void update_terminal_size(frame_params_t *frame_params);

void draw_frame(const frame_params_t *frame_params,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity);

#endif //PIX2ASCII_TERMSTREAM_H
