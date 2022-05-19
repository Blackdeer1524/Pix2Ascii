#ifndef PIX2ASCII_ARGPARSING_H
#define PIX2ASCII_ARGPARSING_H

#include "frame_utils.h"

typedef enum {SOURCE_FILE, SOURCE_CAMERA} source_t;

typedef struct {
    source_t reading_type;
    char *charset_data;
    char *file_path;
    region_intensity_t pixel_block_processing_method;
} user_params_t;

int argparse(user_params_t *user_params, int argc, char *argv[]);


#endif //PIX2ASCII_ARGPARSING_H
