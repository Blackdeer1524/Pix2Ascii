#include <stdio.h>
#include <ncurses.h>

#include "utils.h"
#include <stdlib.h>

#define FRAME_WIDTH 960
#define FRAME_HEIGHT 720
#define VIDEO_FRAMERATE 24
#define TOTAL_READ_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3)




#include <assert.h>
#include <string.h>

char char_set[] = "N@#W$9876543210?!abc;:+=-,._ ";
static unsigned char char_set_len = sizeof(char_set) - 1;

unsigned char get_color_intensity(const unsigned char r,
                                  const unsigned char g,
                                  const unsigned char b) {
    return (r + g + b) / 3;  // ranges from 0 to 255
}

static unsigned char normalization_term = sizeof(char_set) - 2;
char get_char_given_intensity(unsigned char intensity) {
//    int test = intensity * normalization_term / 255;
//    assert((test >= 0) && (test <= char_set_len));
    return char_set[normalization_term - intensity * normalization_term / 255];
}


unsigned char frame[FRAME_HEIGHT][FRAME_WIDTH][3] = {0};

unsigned char get_region_intensity(unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                   unsigned long row_step, unsigned long col_step,
                                   const unsigned char screen_data[FRAME_HEIGHT][FRAME_WIDTH][3]) {
    unsigned int cumulate_brightness = 0;
    int n = 0;
    for (unsigned long i = cur_pixel_row; i < cur_pixel_row + row_step; ++i)
        for (unsigned long j = cur_pixel_col; j < cur_pixel_col + col_step; ++j) {
            cumulate_brightness += screen_data[cur_pixel_row][cur_pixel_col][0];
            cumulate_brightness += screen_data[cur_pixel_row][cur_pixel_col][1];
            cumulate_brightness += screen_data[cur_pixel_row][cur_pixel_col][2];
            n += 3;
        }
//    unsigned int test = cumulate_brightness / (row_step * col_step * 3);
//    assert(test >= 0 && test <= 255);

    return cumulate_brightness / (row_step * col_step * 3);
}


#include <unistd.h>
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Bad number of arguments!\n");
        return -1;
    }
    char command_buffer[256];
    // scale=%d:%d -framerate %d
    int n = sprintf(command_buffer, "ffmpeg -i %s -f image2pipe -hide_banner -loglevel error"
                                    " -vcodec rawvideo -pix_fmt rgb24 -", argv[1]);
    if (n < 0) {
        fprintf(stderr, "Error when entering command!\n");
        return -1;
    }
    FILE *pipein = popen(command_buffer, "r");
    if (!pipein) {
        fprintf(stderr, "Error when obtaining data stream!\n");
        return -1;
    }

//    FILE *pipein = popen("ffmpeg -i BadApple.mp4 -f image2pipe -hide_banner -loglevel error"
//                         " -vcodec rawvideo -pix_fmt rgb24 -", "r");
//    setbuffer(pipein, NULL, TOTAL_READ_SIZE * 100);


    puts("\n");
    initscr();
//    curs_set(0);

    unsigned int n_available_rows, n_available_cols;

    getmaxyx(stdscr, n_available_rows, n_available_cols);
    n_available_cols -= 10;
    unsigned int row_downscale_coef = FRAME_HEIGHT / n_available_rows;
    unsigned int col_downscale_coef = FRAME_WIDTH / n_available_cols;
    char *buffer = calloc(sizeof(char), n_available_cols);

    unsigned long n_read_items;
    unsigned long cur_pixel_row, cur_pixel_col;
    unsigned long current_char_index;

    while ((n_read_items = fread(frame, 1, TOTAL_READ_SIZE, pipein)) == TOTAL_READ_SIZE) {
        clear();
        for (cur_pixel_row=0;
             cur_pixel_row < FRAME_HEIGHT - FRAME_HEIGHT % row_downscale_coef;
             cur_pixel_row += row_downscale_coef) {

            for (current_char_index=0, cur_pixel_col=0;
                 current_char_index < n_available_cols - 1 &&
                 cur_pixel_col < FRAME_WIDTH - FRAME_WIDTH % col_downscale_coef;
                 ++current_char_index,
                 cur_pixel_col += col_downscale_coef)
                buffer[current_char_index] = get_char_given_intensity(
                        get_region_intensity(cur_pixel_row, cur_pixel_col,
                                             row_downscale_coef, col_downscale_coef,
                                             frame));
            buffer[current_char_index] = '\0';
            printw("%s\n", buffer);
        }
        refresh();
        usleep(40000);
    }

    endwin();
    free(buffer);
    fflush(pipein);
    pclose(pipein);
    return 0;
}
