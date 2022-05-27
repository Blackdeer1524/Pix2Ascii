#ifndef PIX2ASCII_ARGPARSING_H
#define PIX2ASCII_ARGPARSING_H

#include "frame_processing.h"

typedef enum {SOURCE_FILE, SOURCE_CAMERA} source_t;

typedef struct {
    char *char_set;
    unsigned int last_index;
} charset_data_t;

typedef struct {
    source_t reading_type;
    char *file_path;
    charset_data_t charset_data;
    region_intensity_t pixel_block_processing_method;
    int color_flag;
    int n_stream_loops;
    char *player_flag;
    kernel_update_method kernel_update;
} user_params_t;

int argparse(user_params_t *user_params, int argc, char *argv[]);


#endif //PIX2ASCII_ARGPARSING_H
