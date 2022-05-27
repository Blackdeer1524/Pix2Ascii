#ifndef PROJECT_INCLUDE_FRAME_UTILS_H_
#define PROJECT_INCLUDE_FRAME_UTILS_H_

typedef struct {
    unsigned char *video_frame;
    int width;
    int height;
    int trimmed_width;  // cached for draw_frame
    int trimmed_height;  // cached for draw_frame
    int triple_width;  // cached for process_block
} frame_params_t;

typedef struct {
    int width;
    int height;
    int volume;  // cached for region_intensity_t: width * height * 3
} kernel_params_t;


void process_block(const frame_params_t *frame_params,
                   const kernel_params_t *kernel_params,
                   int cur_pixel_row,
                   int cur_pixel_col,
                   unsigned int *r, unsigned int *g, unsigned int *b);

typedef unsigned char (*region_intensity_t)(unsigned int r, unsigned int g, unsigned int b,
                                            int normalization);

unsigned char average_chanel_intensity(unsigned int r, unsigned int g, unsigned int b,
                                       int normalization);

unsigned char yuv_intensity(unsigned int r, unsigned int g, unsigned int b,
                            int normalization);

#endif  // PROJECT_INCLUDE_FRAME_UTILS_H_
