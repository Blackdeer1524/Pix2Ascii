#ifndef PIX2ASCII_STRUCT_H
#define PIX2ASCII_STRUCT_H

#include "frame_utils.h"
#include "video_stream.h"

typedef struct {
    source_t reading_type;
    char *charset_data;
    char *file_path;
    region_intensity_t pixel_block_processing_method;
} user_params_t;

typedef struct {
    unsigned char *video_frame;
    int width;
    int height;
} frame_data_t;

#endif //PIX2ASCII_STRUCT_H
