#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"
#include <stdlib.h>

#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720       // 1280    16
#define ASPECT_RATIO_WIDTH 16  // ---- =  --
#define ASPECT_RATIO_HEIGHT 9  //  720     9


#define VIDEO_FRAMERATE 24
#define TOTAL_READ_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3)
#define FRAME_TIMING_SLEEP 1000000 / VIDEO_FRAMERATE

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//
typedef struct {
    char *char_set;
    unsigned int last_index;
} char_set_data;

typedef enum {CHARSET_SHORT, CHARSET_MEDIUM, CHARSET_LONG, CHARSET_N} t_char_set;
static char_set_data char_sets[CHARSET_N] = {
        {"@%#*+=-:. ", 9},
        {"N@#W$9876543210?!abc;:+=-,._ ", 28},
        {"$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ", 69}};

#include <assert.h>
char get_char_given_intensity(unsigned char intensity, const char *char_set, unsigned int max_index) {
    unsigned int test = max_index - intensity * max_index / 255;
    assert(0 <= test && test <= max_index);
    assert(char_set[test] != '\0');
    return char_set[max_index - intensity * max_index / 255];
}


unsigned char frame[FRAME_HEIGHT][FRAME_WIDTH][3] = {0};

unsigned char get_region_intensity(unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                   unsigned long row_step, unsigned long col_step,
                                   const unsigned char screen_data[FRAME_HEIGHT][FRAME_WIDTH][3]) {
    unsigned int cumulate_brightness = 0;
    for (unsigned long i = cur_pixel_row; i < cur_pixel_row + row_step; ++i)
        for (unsigned long j = cur_pixel_col; j < cur_pixel_col + col_step; ++j) {
            cumulate_brightness += screen_data[cur_pixel_row][cur_pixel_col][0];
            cumulate_brightness += screen_data[cur_pixel_row][cur_pixel_col][1];
            cumulate_brightness += screen_data[cur_pixel_row][cur_pixel_col][2];
        }
    return cumulate_brightness / (row_step * col_step * 3);
}


unsigned long micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    unsigned long us = 1000000 * (uint64_t) ts.tv_sec + (uint64_t) ts.tv_nsec / 1000;
    return us;
}

typedef enum {SOURCE_FILE, SOURCE_CAMERA} t_source;

#include <assert.h>
void prepare_terminal_size(unsigned int *char_height, unsigned int *char_width) {
    unsigned long new_height = *char_width * ASPECT_RATIO_HEIGHT / ASPECT_RATIO_WIDTH;
    if (new_height > *char_height) {
        *char_width = *char_height * ASPECT_RATIO_WIDTH / ASPECT_RATIO_HEIGHT;
        return;
    }
    *char_height = new_height;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Bad number of arguments!\n");
        return -1;
    }

//    -c: camera
//    -f "filepath"
//    -color: char set to choose
    t_source reading_type = SOURCE_FILE;
    char *filepath = NULL;
    t_char_set picked_char_set_type = CHARSET_MEDIUM;

    for (int i=1; i<argc;) {
        if (argv[i][0] != '-') {
            fprintf(stderr, "Invalid argument! Value is given without a corresponding flag!\n");
            return -1;
        }

        if (!strcmp(&argv[i][1], "c")) {
            reading_type = SOURCE_CAMERA;
            ++i;
        } else if (!strcmp(&argv[i][1], "f")) {
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                fprintf(stderr, "Invalid argument! Filepath is not given!\n");
                return -1;
            } else {
                reading_type = SOURCE_FILE;
                filepath = argv[i + 1];
                i += 2;
            }
        } else if (!strcmp(&argv[i][1], "color")){
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                fprintf(stderr, "Invalid argument! Color scheme is not given!\n");
                return -1;
            }

            if (!strcmp(argv[i + 1], "short")){
                picked_char_set_type = CHARSET_SHORT;
            } else if (!strcmp(argv[i + 1], "medium")){
                picked_char_set_type = CHARSET_MEDIUM;
            } else if (!strcmp(argv[i + 1], "long")){
                picked_char_set_type = CHARSET_LONG;
            } else {
                fprintf(stderr, "Invalid argument! Unsupported scheme!\n");
                return -1;
            }
            i += 2;
        } else {
            fprintf(stderr, "Unknown flag!\n");
            return -1;
        }
    }

    char command_buffer[512];
    int n = 0;
    if (reading_type == SOURCE_CAMERA)
        n = sprintf(command_buffer, "ffmpeg -hide_banner -loglevel error "
                                    "-f v4l2 -i /dev/video0 -f image2pipe "
                                    "-vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -",
                    VIDEO_FRAMERATE, FRAME_WIDTH, FRAME_HEIGHT);
    else if (reading_type == SOURCE_FILE)
        n = sprintf(command_buffer, "ffmpeg -i %s -f image2pipe -hide_banner -loglevel error "
                                    "-vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -",
                    filepath, VIDEO_FRAMERATE, FRAME_WIDTH, FRAME_HEIGHT);
    if (n < 0) {
        fprintf(stderr, "Error writing ffmpeg query!\n");
        return -1;
    }

    char *char_set = char_sets[picked_char_set_type].char_set;
    unsigned int max_char_set_index = char_sets[picked_char_set_type].last_index;

    FILE *pipein = popen(command_buffer, "r");
    if (!pipein) {
        fprintf(stderr, "Error when obtaining data stream!\n");
        return -1;
    }
    initscr();
    curs_set(0);

    unsigned int n_available_rows=0, n_available_cols=0;
    unsigned int new_n_available_rows=1, new_n_available_cols=1;

    unsigned long cur_pixel_row, cur_pixel_col;
    unsigned long cur_char_row_index, cur_char_col_index;

    unsigned int row_downscale_coef = 1;
    unsigned int col_downscale_coef = 1;

    unsigned long n_read_items;
    unsigned int offset;
    char *buffer = NULL;
    unsigned long t;

    FILE *test = fopen("./test_data.txt", "w");
    while (1) {
        // when terminal is being resized, you can NOT read anything from camera
        // (don't know why). This results in n_read_items = 0.
        n_read_items = fread(frame, 1, TOTAL_READ_SIZE, pipein);

        t = micros();
        getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
        if (n_available_rows != new_n_available_rows ||
            n_available_cols != new_n_available_cols) {
            if (!n_read_items)
                continue;

            n_available_rows = new_n_available_rows;
            n_available_cols = new_n_available_cols;
            row_downscale_coef = (FRAME_HEIGHT + n_available_rows) / n_available_rows;  // MAX(, 1);
            col_downscale_coef = (FRAME_WIDTH  + n_available_cols) / n_available_cols;  // MAX(, 1);
            free(buffer);
            buffer = calloc(sizeof(char), n_available_cols * n_available_rows);
        } else if (!n_read_items) {  // <== this if statement is needed for camera to work properly.
            break;                   // stops when you couldn't read anything even if you didn't resize terminal.
        }

        int line_term_n = 0;
        offset = 0;
        for (cur_char_row_index=0, cur_pixel_row=0;
             cur_char_row_index < n_available_rows - 1 &&
             cur_pixel_row < FRAME_HEIGHT - FRAME_HEIGHT % row_downscale_coef;
             ++cur_char_row_index,
             cur_pixel_row += row_downscale_coef) {

            for (cur_char_col_index=0, cur_pixel_col=0;
                 cur_char_col_index < n_available_cols - 1 &&
                 cur_pixel_col < FRAME_WIDTH - FRAME_WIDTH % col_downscale_coef;
                 ++cur_char_col_index,
                         cur_pixel_col += col_downscale_coef)
                buffer[offset + cur_char_col_index] = get_char_given_intensity(
                        get_region_intensity(cur_pixel_row, cur_pixel_col,
                                             row_downscale_coef, col_downscale_coef,
                                             frame), char_set, max_char_set_index);
            for (unsigned int j = offset + cur_char_col_index; j < offset + n_available_cols - 1; ++j)
                buffer[j] = ' ';
            offset += n_available_cols;
            buffer[offset-1] = '\n';
            ++line_term_n;
        }
        buffer[offset-1] = '\0';
        move(0, 0);
        printw("%s\n", buffer);
        refresh();
        t = micros() - t;
        usleep(FRAME_TIMING_SLEEP - t);
    }
    fclose(test);
    free(buffer);
    getchar();
    endwin();
    fflush(pipein);
    pclose(pipein);
    return 0;
}
