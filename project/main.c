#include <stdio.h>
#include <ncurses.h>

#include "utils.h"
#include <stdlib.h>

#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720
#define VIDEO_FRAMERATE 24
#define TOTAL_READ_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3)
#define FRAME_TIMING_SLEEP 1000000 / VIDEO_FRAMERATE


// $@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\|()1{}[]?-_+~<>i!lI;:,"^`'.
char char_set[] = "N@#W$9876543210?!abc;:+=-,._ ";


static unsigned char char_set_len = sizeof(char_set) - 1;

static unsigned char normalization_term = sizeof(char_set) - 2;
char get_char_given_intensity(unsigned char intensity) {
    return char_set[normalization_term - intensity * normalization_term / 255];
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

#include <time.h>

unsigned long micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    unsigned long us = 1000000 * (uint64_t) ts.tv_sec + (uint64_t) ts.tv_nsec / 1000;
    return us;
}


#include <unistd.h>
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Bad number of arguments!\n");
        return -1;
    }

    char command_buffer[256];
    // scale=%d:%d -framerate %d

//    "ffmpeg -i %s -f image2pipe -hide_banner -loglevel error"
//    -                                    " -vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -"
    int n = sprintf(command_buffer, "ffmpeg -hide_banner -loglevel error "
                                    "-f v4l2 -i /dev/video0 -f image2pipe "
                                    "-vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -",
                                    VIDEO_FRAMERATE, FRAME_WIDTH, FRAME_HEIGHT);
    if (n < 0) {
        fprintf(stderr, "Error when entering command!\n");
        return -1;
    }
    FILE *pipein = popen(command_buffer, "r");
    if (!pipein) {
        fprintf(stderr, "Error when obtaining data stream!\n");
        return -1;
    }
    puts("\n");
    initscr();
    curs_set(0);

    unsigned int n_available_rows=0, n_available_cols=0;
    unsigned int new_n_available_rows=1, new_n_available_cols=1;

    unsigned long cur_pixel_row, cur_pixel_col;
    unsigned long cur_char_row_index, cur_char_col_index;

    unsigned int row_downscale_coef = 0;
    unsigned int col_downscale_coef = 0;

    unsigned long n_read_items;
    unsigned int offset;
    char *buffer = NULL;
    unsigned long t;

    while (1) {
        if (!fread(frame, 1, TOTAL_READ_SIZE, pipein))
            continue;

        t = micros();
        getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
        new_n_available_cols -= 10;
        if (n_available_rows != new_n_available_rows ||
            n_available_cols != new_n_available_cols) {

            n_available_rows = new_n_available_rows;
            n_available_cols = new_n_available_cols;
            row_downscale_coef = FRAME_HEIGHT / n_available_rows;
            col_downscale_coef = FRAME_WIDTH / n_available_cols;
            free(buffer);
            buffer = calloc(sizeof(char), n_available_cols * n_available_rows);
        }
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
                                             frame));
            buffer[offset + cur_char_col_index] = '\n';
            offset += n_available_cols;
        }
        buffer[offset] = '\0';
        move(0, 0);
        printw("%s\n", buffer);
        refresh();
        t = micros() - t;
        usleep(FRAME_TIMING_SLEEP - t);
    }
    free(buffer);
    getchar();
    endwin();
    fflush(pipein);
    pclose(pipein);
    return 0;
}
