#ifndef PIX2ASCII_ARGPARSING_H
#define PIX2ASCII_ARGPARSING_H

#include "frame_processing.h"

typedef enum {SOURCE_FILE, SOURCE_CAMERA} source_t;

typedef struct {
    char *char_set;
    unsigned int last_index;
} charset_params_t;

typedef struct {
    charset_params_t charset_params;

    struct {
        source_t reading_type;
        char *file_path;
        int n_stream_loops;
        char *player_flag;
    } ffmpeg_params;

    struct {
        region_intensity_t rgb_channels_processor;
        kernel_update_method update_kernel;
    } frame_processing_params;

    struct {
        int color_flag;
        int max_width;
        int min_width;
        int max_height;
        int min_height;
    } terminal_params;
} user_params_t;

int argparse(user_params_t *user_params, int argc, char *argv[]);


#endif //PIX2ASCII_ARGPARSING_H
