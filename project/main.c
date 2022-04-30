#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define VIDEO_FRAMERATE 24
#define FRAME_TIMING_SLEEP 1000000 / VIDEO_FRAMERATE
#define COMMAND_BUFFER_SIZE 512

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


typedef unsigned char (*region_intensity_t)(const unsigned char *frame,
                                            unsigned int frame_width,
                                            unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                            unsigned long row_step, unsigned long col_step);

static unsigned int rgb[3];
int process_block(const unsigned char *frame,
                  unsigned int frame_width,
                  unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                  unsigned long row_step, unsigned long col_step){
    static unsigned int local_r, local_g, local_b;
    local_r=0, local_g=0, local_b=0;

    static unsigned int down_row, right_col;
    down_row = cur_pixel_row + row_step;
    right_col = cur_pixel_col + col_step;

    static unsigned int cashed_frame_width;
    static unsigned int triple_width;
    if (!cashed_frame_width) {
        triple_width = frame_width * 3;
        cashed_frame_width = frame_width;
    }

    static unsigned int triple_cur_pixel_col;
    triple_cur_pixel_col = cur_pixel_col * 3;

    static unsigned int row_offset, col_offset;
    static unsigned int i, j;
    for (row_offset = triple_width * cur_pixel_row, i = cur_pixel_row;
         i < down_row;
         row_offset += triple_width, ++i) {
        for (col_offset = triple_cur_pixel_col, j = cur_pixel_col;
             j < right_col;
             col_offset += 3, ++j) {
            local_r += frame[row_offset + col_offset];
            local_g += frame[row_offset + col_offset + 1];
            local_b += frame[row_offset + col_offset + 2];
        }
    }
    rgb[0] = local_r;
    rgb[1] = local_g;
    rgb[2] = local_b;
    return 0;
}

unsigned char average_chanel_intensity(const unsigned char *frame,
                                       unsigned int frame_width,
                                       unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                       unsigned long row_step, unsigned long col_step) {
    process_block(frame, frame_width, cur_pixel_row, cur_pixel_col,
                                      row_step,      col_step);
    return (rgb[0] + rgb[1] + rgb[2]) / (row_step * col_step * 3);
}

unsigned char yuv_intensity(const unsigned char *frame,
                            unsigned int frame_width,
                            unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                            unsigned long row_step, unsigned long col_step) {
    process_block(frame, frame_width, cur_pixel_row, cur_pixel_col,
                  row_step,      col_step);
    double test = (rgb[0] * 0.299 + 0.587 * rgb[1] + 0.114 * rgb[2]) / (row_step * col_step * 3);
    return (unsigned char) MIN(test, 255);
}


unsigned long micros() {
    static struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return 1000000 * ts.tv_sec + ts.tv_nsec / 1000;
}

typedef enum {SOURCE_FILE, SOURCE_CAMERA} t_source;

static unsigned int n_available_rows = 0, n_available_cols = 0;
static unsigned int row_downscale_coef = 1, col_downscale_coef = 1;

void draw_frame(const unsigned char *frame,
                unsigned int frame_width,
                unsigned int frame_height,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity) {

    static unsigned int new_n_available_rows, new_n_available_cols;
    static unsigned int trimmed_height;
    static unsigned int trimmed_width;
    static unsigned int offset;

    getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
    if (n_available_rows != new_n_available_rows ||
        n_available_cols != new_n_available_cols) {
        n_available_rows = new_n_available_rows;
        n_available_cols = new_n_available_cols;
        row_downscale_coef = MAX((frame_height + n_available_rows) / n_available_rows, 1);
        col_downscale_coef = MAX((frame_width  + n_available_cols) / n_available_cols, 1);

        trimmed_height = frame_height - frame_height % row_downscale_coef;
        trimmed_width = frame_width - frame_width % col_downscale_coef;
        offset = (n_available_cols - trimmed_width / col_downscale_coef) / 2;
        clear();
    }

    static unsigned int cur_char_row;
    static unsigned int cur_pixel_row, cur_pixel_col;
    for (cur_char_row=0, cur_pixel_row=0;
         cur_pixel_row < trimmed_height;
         ++cur_char_row,
         cur_pixel_row += row_downscale_coef) {
        move(cur_char_row, offset);
        for (cur_pixel_col=0;
             cur_pixel_col < trimmed_width;
             cur_pixel_col += col_downscale_coef)
            addch(get_char_given_intensity(get_region_intensity(frame,
                                                                frame_width,
                                                                cur_pixel_row, cur_pixel_col,
                                                                row_downscale_coef, col_downscale_coef),
                                           char_set, max_char_set_index));
    }
    refresh();
}


void close_pipe(FILE *pipeline) {
    fflush(pipeline);
    pclose(pipeline);
}

void free_space(unsigned char *frame, FILE *pipeline){
    free(frame);
    close_pipe(pipeline);
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
    region_intensity_t grayscale_method = average_chanel_intensity;

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
        } else if (!strcmp(&argv[i][1], "color")) {
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                fprintf(stderr, "Invalid argument! Color scheme is not given!\n");
                return -1;
            }

            if (!strcmp(argv[i + 1], "sharp")) {
                picked_char_set_type = CHARSET_SHARP;
            } else if (!strcmp(argv[i + 1], "optimal")) {
                picked_char_set_type = CHARSET_OPTIMAL;
            } else if (!strcmp(argv[i + 1], "standart")) {
                picked_char_set_type = CHARSET_STANDART;
            } else {
                fprintf(stderr, "Invalid argument! Unsupported scheme!\n");
                return -1;
            }
            i += 2;
        } else if (!strcmp(&argv[i][1], "method")) {
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                fprintf(stderr, "Invalid argument! Color scheme is not given!\n");
                return -1;
            }

            if (!strcmp(argv[i + 1], "average")) {
                grayscale_method = average_chanel_intensity;
            } else if (!strcmp(argv[i + 1], "yuv")) {
                grayscale_method = yuv_intensity;
            } else {
                fprintf(stderr, "Invalid argument! Unsupported grayscale method!\n");
                return -1;
            }
            i += 2;
        } else {
            fprintf(stderr, "Unknown flag!\n");
            return -1;
        }
    }

    char *char_set = char_sets[picked_char_set_type].char_set;
    unsigned int max_char_set_index = char_sets[picked_char_set_type].last_index;

    char command_buffer[COMMAND_BUFFER_SIZE];
    int n_chars_printed = 0;

    sprintf(command_buffer, "ffplay %s -hide_banner -loglevel error", filepath);
    popen(command_buffer, "r");

    unsigned int FRAME_WIDTH = 1280, FRAME_HEIGHT = 720;
    // obtaining an interface with ffmpeg
    if (reading_type == SOURCE_CAMERA) {
        n_chars_printed = sprintf(command_buffer, "ffmpeg -hide_banner -loglevel error "
                                                  "-f v4l2 -i /dev/video0 -f image2pipe "
                                                  "-vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -",
                                  VIDEO_FRAMERATE, FRAME_WIDTH, FRAME_HEIGHT);
    } else if (reading_type == SOURCE_FILE) {
        // obtaining input resolution
        n_chars_printed = sprintf(command_buffer, "ffprobe -v error -select_streams v:0 "
                                                  "-show_entries stream=width,height -of default=nw=1:nk=1 %s",
                                  filepath);

        if (n_chars_printed < 0) {
            fprintf(stderr, "Error obtaining input resolution!\n");
            return -1;
        } else if (n_chars_printed >= COMMAND_BUFFER_SIZE) {
            fprintf(stderr, "Error obtaining input resolution! Query is too big!\n");
            return -1;
        }

        FILE *image_data_pipe = popen(command_buffer, "r");
        if (!image_data_pipe) {
            fprintf(stderr, "Error obtaining input resolution! Couldn't get an interface with ffprobe!\n");
            return -1;
        }

        if (fscanf(image_data_pipe, "%u %u", &FRAME_WIDTH, &FRAME_HEIGHT) != 2 ||
            fflush(image_data_pipe) || fclose(image_data_pipe)) {
            fprintf(stderr, "Error obtaining input resolution! Width/height not found\n");
            return -1;
        }

        n_chars_printed = sprintf(command_buffer, "ffmpeg -i %s -f image2pipe -hide_banner -loglevel error "
                                                  "-vf fps=%d -vcodec rawvideo -pix_fmt rgb24 -",
                                  filepath, VIDEO_FRAMERATE);
    }

    if (n_chars_printed < 0) {
        fprintf(stderr, "Error preparing ffmpeg command!\n");
        return -1;
    } else if (n_chars_printed >= COMMAND_BUFFER_SIZE) {
        fprintf(stderr, "Error preparing ffmpeg command! Query size is too big!\n");
        return -1;
    }

    FILE *pipein = popen(command_buffer, "r");
    if (!pipein) {
        fprintf(stderr, "Error when obtaining data stream! Couldn't get an interface with ffmpeg!\n");
        return -1;
    }

    unsigned char *frame = malloc(sizeof(char) * FRAME_HEIGHT * FRAME_WIDTH * 3);
    unsigned int TOTAL_READ_SIZE = (FRAME_WIDTH * FRAME_HEIGHT * 3);

    initscr();
    curs_set(0);

    char test[100000];
    setbuf(stdout, test);

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
//    popen("ffplay ./BadApple.mp4 -vn -nodisp -hide_banner -loglevel error", "r");
//    FILE *t = popen("ffprobe -select_streams v -show_entries frame=pict_type -of csv ./BadApple.mp4"
//                    " -hide_banner -loglevel error", "r");
//    printf("<dfgsfdsdfhsdfgsdfgse>%s", command_buffer);
//    exit(0);
    while ((start = micros()) && ((n_read_items = fread(frame, 1, TOTAL_READ_SIZE, pipein)))) {

        if (n_read_items < TOTAL_READ_SIZE) {
            usleep(FRAME_TIMING_SLEEP - (micros() - start));
            continue;
        }

        draw_frame(frame, FRAME_WIDTH, FRAME_HEIGHT, char_set, max_char_set_index, grayscale_method);
        // compensates time loss (?)
        if (FRAME_TIMING_SLEEP <= (frame_proc_time = micros() - start)) {
            total_loosed_frames = (double) (frame_proc_time - FRAME_TIMING_SLEEP) / FRAME_TIMING_SLEEP;
            n_frames_to_skip = frame_proc_time / FRAME_TIMING_SLEEP;
            n_loosed_frames =  (frame_proc_time - FRAME_TIMING_SLEEP) / FRAME_TIMING_SLEEP;
            fseek(pipein, TOTAL_READ_SIZE * n_frames_to_skip, SEEK_CUR);
            usleep((unsigned int) (FRAME_TIMING_SLEEP * (total_loosed_frames - n_loosed_frames)));
            continue;
        }
        usleep(FRAME_TIMING_SLEEP - (micros() - start));
    }
    getchar();
    free_space(frame, pipein);
    endwin();
    return 0;
}
