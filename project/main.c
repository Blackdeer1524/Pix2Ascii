#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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

typedef enum {CHARSET_SHARP, CHARSET_OPTIMAL, CHARSET_STANDART, CHARSET_N} t_char_set;
static char_set_data char_sets[CHARSET_N] = {
        {"@%#*+=-:. ", 9},
        {"NBUa1|^` ", 8},
        {"N@#W$9876543210?!abc;:+=-,._ ", 28},
        };

#include <assert.h>
char get_char_given_intensity(unsigned char intensity, const char *char_set, unsigned int max_index) {
    return char_set[max_index - intensity * max_index / 255];
}


static unsigned char frame[FRAME_HEIGHT][FRAME_WIDTH][3] = {0};

typedef unsigned char (*region_intensity_t)(unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                   unsigned long row_step, unsigned long col_step);

unsigned char average_intensity(unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                   unsigned long row_step, unsigned long col_step) {
    unsigned int cumulate_brightness = 0;
    for (unsigned long i = cur_pixel_row; i < cur_pixel_row + row_step; ++i)
        for (unsigned long j = cur_pixel_col; j < cur_pixel_col + col_step; ++j) {
            cumulate_brightness += frame[cur_pixel_row][cur_pixel_col][0];
            cumulate_brightness += frame[cur_pixel_row][cur_pixel_col][1];
            cumulate_brightness += frame[cur_pixel_row][cur_pixel_col][2];
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

static unsigned int n_available_rows = 0, n_available_cols = 0;
static unsigned int row_downscale_coef = 1, col_downscale_coef = 1;

void draw_frame(const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity) {

    static unsigned int new_n_available_rows, new_n_available_cols;
    static unsigned int cur_pixel_row, cur_pixel_col;

    getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
    if (n_available_rows != new_n_available_rows ||
        n_available_cols != new_n_available_cols) {
        n_available_rows = new_n_available_rows;
        n_available_cols = new_n_available_cols;
        row_downscale_coef = MAX((FRAME_HEIGHT + n_available_rows) / n_available_rows, 1);
        col_downscale_coef = MAX((FRAME_WIDTH  + n_available_cols) / n_available_cols, 1);
    }

    for (cur_pixel_row=0;
         cur_pixel_row < FRAME_HEIGHT - FRAME_HEIGHT % row_downscale_coef;
         cur_pixel_row += row_downscale_coef) {

        for (cur_pixel_col=0;
             cur_pixel_col < FRAME_WIDTH - FRAME_WIDTH % col_downscale_coef;
             cur_pixel_col += col_downscale_coef)
            addch(get_char_given_intensity(get_region_intensity(cur_pixel_row, cur_pixel_col,
                                                                row_downscale_coef, col_downscale_coef),
                                           char_set, max_char_set_index));
        addch('\n');
    }
    move(0, 0);
    refresh();
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
    t_char_set picked_char_set_type = CHARSET_OPTIMAL;

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

            if (!strcmp(argv[i + 1], "sharp")){
                picked_char_set_type = CHARSET_SHARP;
            } else if (!strcmp(argv[i + 1], "optimal")){
                picked_char_set_type = CHARSET_OPTIMAL;
            } else if (!strcmp(argv[i + 1], "standart")){
                picked_char_set_type = CHARSET_STANDART;
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

    unsigned long n_read_items;  // n bytes read from pipe
    double total_loosed_frames;  // fraction representation of loosed frames
    unsigned int n_loosed_frames;  // floor (total loosed frames). This var corresponds to the number of
                                   // WHOLE frames loosed while processing current one
    unsigned long long start, frame_proc_time = 1;  // frame processing time vars
    unsigned int n_frames_to_skip = 0;  // ceil (total loosed frames). Suppose that we loosed 1.5 frames
                                        // Hence to synchronize our stream we have to omit ceil(1.5) = 2 frames

    // !(ferror(pipein) || (!n_read_items && feof(pipein))) => (!ferror(pipein) && !(!n_read_items && feof(pipein))) =>
    // (!ferror(pipein) && (n_read_items || !feof(pipein)))
//    while (!ferror(pipein) && ((n_read_items = fread(frame, 1, TOTAL_READ_SIZE, pipein)) || !feof(pipein))) {

    while ((n_read_items = fread(frame, 1, TOTAL_READ_SIZE, pipein)) || !feof(pipein)) {
        start = micros();
        if (n_read_items < TOTAL_READ_SIZE)
            continue;

        draw_frame(char_set, max_char_set_index, average_intensity);
        frame_proc_time = micros() - start;
        // compensates time loss (?)
        if (FRAME_TIMING_SLEEP < frame_proc_time) {
            total_loosed_frames = (double) (frame_proc_time - FRAME_TIMING_SLEEP) / FRAME_TIMING_SLEEP;
            n_frames_to_skip = frame_proc_time / FRAME_TIMING_SLEEP;
            n_loosed_frames =  (frame_proc_time - FRAME_TIMING_SLEEP) / FRAME_TIMING_SLEEP;
            fseek(pipein, TOTAL_READ_SIZE * n_frames_to_skip, SEEK_CUR);
            usleep((unsigned int) (FRAME_TIMING_SLEEP * (total_loosed_frames - n_loosed_frames)));
            continue;
        }
        usleep(FRAME_TIMING_SLEEP - frame_proc_time);
    }
    getchar();
    endwin();
    fflush(pipein);
    pclose(pipein);
    return 0;
}
