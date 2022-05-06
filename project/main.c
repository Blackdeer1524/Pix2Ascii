#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>

#define N_uSECONDS_IN_ONE_SEC 1000000
#define COMMAND_BUFFER_SIZE 512
#define VIDEO_FRAMERATE 25

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct {
    char *char_set;
    unsigned int last_index;
} char_set_data;

typedef enum {CHARSET_SHARP, CHARSET_OPTIMAL, CHARSET_STANDART, CHARSET_LONG, CHARSET_N} t_char_set;
static char_set_data char_sets[CHARSET_N] = {
        {"@%#*+=-:. ", 9},
        {"NBUa1|^` ", 8},
        {"N@#W$9876543210?!abc;:+=-,._ ", 28},
        {"$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ", 69}
};

char get_char_given_intensity(unsigned char intensity, const char *char_set, unsigned int max_index) {
    return char_set[max_index - intensity * max_index / 255];
}


typedef unsigned char (*region_intensity_t)(const unsigned char *rgb_data,
                                            unsigned int frame_width,
                                            unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                            unsigned long row_step, unsigned long col_step);

static unsigned int rgb[3];
int process_block(const unsigned char *rgb_data,
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
            local_r += rgb_data[row_offset + col_offset];
            local_g += rgb_data[row_offset + col_offset + 1];
            local_b += rgb_data[row_offset + col_offset + 2];
        }
    }
    rgb[0] = local_r;
    rgb[1] = local_g;
    rgb[2] = local_b;
    return 0;
}

unsigned char average_chanel_intensity(const unsigned char *rgb_data,
                                       unsigned int frame_width,
                                       unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                                       unsigned long row_step, unsigned long col_step) {
    process_block(rgb_data, frame_width, cur_pixel_row, cur_pixel_col,
                  row_step,      col_step);
    return (rgb[0] + rgb[1] + rgb[2]) / (row_step * col_step * 3);
}

unsigned char yuv_intensity(const unsigned char *rgb_data,
                            unsigned int frame_width,
                            unsigned long cur_pixel_row,  unsigned long cur_pixel_col,
                            unsigned long row_step, unsigned long col_step) {
    process_block(rgb_data, frame_width, cur_pixel_row, cur_pixel_col,
                  row_step,      col_step);
    double current_block_intensity = (rgb[0] * 0.299 + 0.587 * rgb[1] + 0.114 * rgb[2]) / (row_step * col_step * 3);
    return (unsigned char) MIN(current_block_intensity, 255);
}

// Timer =============================================
typedef struct timespec timespec;
timespec startTime;

timespec diff(timespec *start, timespec *end) {
    static timespec temp;
    temp.tv_sec = end->tv_sec - start->tv_sec;
    if ((end->tv_nsec < start->tv_nsec)) {
        --temp.tv_sec;
        temp.tv_nsec = N_uSECONDS_IN_ONE_SEC * 1000 + end->tv_nsec - start->tv_nsec;
    } else {
        temp.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return temp;
}

// total time elapsed from start
uint64_t get_elapsed_time_from_start_us(){
    static timespec tmpTime, diffTime;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &tmpTime);
    diffTime = diff(&startTime, &tmpTime);
    return  ((uint64_t)diffTime.tv_sec * N_uSECONDS_IN_ONE_SEC * 1000 + (uint64_t)diffTime.tv_nsec) / 1000;
}

int set_timer() {
    clock_gettime(CLOCK_MONOTONIC_COARSE, &startTime);
    return 0;
}

// =============================================================


typedef enum {SOURCE_FILE, SOURCE_CAMERA} t_source;

static unsigned int n_available_rows = 0, n_available_cols = 0;
static unsigned int row_downscale_coef = 1, col_downscale_coef = 1;

void draw_frame(const unsigned char *rgb_data,
                unsigned int frame_width,
                unsigned int frame_height,
                const char char_set[],
                unsigned int max_char_set_index,
                region_intensity_t get_region_intensity) {

    static unsigned int new_n_available_rows, new_n_available_cols;
    static unsigned int trimmed_height, trimmed_width;
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
            addch(get_char_given_intensity(get_region_intensity(rgb_data,
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

void free_space(unsigned char *rgb_data, FILE *or_source, FILE *pipeline, FILE *logs_file){
    free(rgb_data);
    close_pipe(or_source);
    close_pipe(pipeline);
    fflush(logs_file);
    fclose(logs_file);
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

    // argument parsing
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
            } else if (!strcmp(argv[i + 1], "long")) {
                picked_char_set_type = CHARSET_LONG;
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
    int n_chars_printed;

    unsigned int FRAME_WIDTH = 1280, FRAME_HEIGHT = 720;
    // obtaining an interface with ffmpeg
    if (reading_type == SOURCE_CAMERA) {
        n_chars_printed = sprintf(command_buffer, "ffmpeg -hide_banner -loglevel error "
                                                  "-f v4l2 -i /dev/video0 -f image2pipe "
                                                  "-vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -",
                                  VIDEO_FRAMERATE, FRAME_WIDTH, FRAME_HEIGHT);
    } else if (reading_type == SOURCE_FILE) {
        // get input resolution command
        n_chars_printed = sprintf(command_buffer, "ffprobe -v error -select_streams v:0"
                                                  " -show_entries stream=width,height"
                                                  " -of default=noprint_wrappers=1:nokey=1 %s",
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
        // reading input resolution
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

    // sets up stream from where we read our RGB frames
    FILE *pipein = popen(command_buffer, "r");
    if (!pipein) {
        fprintf(stderr, "Error when obtaining data stream! Couldn't get an interface with ffmpeg!\n");
        return -1;
    }

    unsigned char *rgb_data = malloc(sizeof(char) * FRAME_HEIGHT * FRAME_WIDTH * 3);
    uint64_t TOTAL_READ_SIZE = (FRAME_WIDTH * FRAME_HEIGHT * 3);

    uint64_t n_read_items;  // n bytes read from pipe
    uint64_t prev_frame_index = 0, current_frame_index = 0;
    uint64_t next_frame_index_measured_by_time = 0;
    int64_t desync=0, i;

    uint64_t total_elapsed_time, last_total_elapsed_time=0;
    uint64_t frame_timing_sleep = N_uSECONDS_IN_ONE_SEC / VIDEO_FRAMERATE;  // uint64_t int division doesn't work with operand of other type
    uint64_t usecs_per_frame=0, sleep_time;

    FILE *logs = fopen("Logs.txt", "w");
    // aligning player and program output ====================
    // We force ffplay (our video player) to write its log to a file called <StartIndicator>.
    // It looks like this:

    // ffplay started on 2022-05-06 at 21:39:01
    //Report written to "StartIndicator"
    //Log level: 32
    //Command line:
    //ffplay <filepath> -hide_banner -loglevel error -nostats -vf showinfo
    //[Parsed_showinfo_0 @ 0x7fc458002f00] config in time_base: 1/1000, frame_rate: 30/1
    //[Parsed_showinfo_0 @ 0x7fc458002f00] config out time_base: 0/0, frame_rate: 0/0
    //[Parsed_showinfo_0 @ 0x7fc458002f00] n:   0 pts:      0 pts_time:0       pos:      ...
    // ...

    // When ffplay loads and starts to process video frames it logs it in the THIRD (check needed) line
    // that starts with symbol '['. That symbol we are constantly looking for.

    FILE *original_source = NULL;
    if (reading_type == SOURCE_FILE) {
        FILE *ffplay_log_file = fopen("StartIndicator", "w");
        assert(ffplay_log_file);
        fclose(ffplay_log_file);
        sprintf(command_buffer, "FFREPORT=file=StartIndicator:level=32 "
                                "ffplay %s -hide_banner -loglevel error -nostats -vf showinfo", filepath);
        original_source = popen(command_buffer, "r");

        ffplay_log_file = fopen("StartIndicator", "r");
        char current_file_char;
        int n_bracket_encounters = 0;
        long int current_file_position;

        while (n_bracket_encounters < 3) {
            if ((current_file_char = getc(ffplay_log_file)) == EOF) {
                current_file_position = ftell(ffplay_log_file)-1;
                fclose(ffplay_log_file);
                ffplay_log_file = fopen("StartIndicator", "r");
                fseek(ffplay_log_file, current_file_position, SEEK_SET);
            }
            else if (current_file_char == '[')
                ++n_bracket_encounters;
        }
        fclose(ffplay_log_file);
    }
    // ==============================
    set_timer();

    initscr();
    curs_set(0);
    total_elapsed_time = get_elapsed_time_from_start_us();
    while ((n_read_items = fread(rgb_data, sizeof(char), TOTAL_READ_SIZE, pipein)) || !feof(pipein)) {
        if (n_read_items < TOTAL_READ_SIZE) {
            total_elapsed_time = get_elapsed_time_from_start_us();
            sleep_time = frame_timing_sleep - (total_elapsed_time % frame_timing_sleep);
            usleep(sleep_time);
            continue;
        }
        ++current_frame_index;  // current_frame_index is incremented because of fread()
        draw_frame(rgb_data, FRAME_WIDTH, FRAME_HEIGHT, char_set, max_char_set_index, grayscale_method);
        // =============================================
        // debug info about PREVIOUS frame
        // EL uS    - elapsed time (in microseconds) from the start;
        // EL S     - elapsed time (in seconds) from the start;
        // FI       - current Frame Index;
        // TFI      - current Frame Index measured by elapsed time;
        // uSPF     - micro (u) Seconds Per Frame (Canonical value);
        // Cur uSPF - micro (u) Seconds Per Frame (current);
        // Avg uSPF - micro (u) Seconds Per Frame (Avg);
        // FPS      - Frames Per Second;
        sprintf(command_buffer,
                "EL uS:%10" PRIu64 "|EL S:%8.2Lf|FI:%5" PRIu64 "|TFI:%5" PRIu64 "|TFI - FI:%2" PRId64
        "|uSPF:%8" PRIu64 "|Cur uSPF:%8" PRIu64 "|Avg uSPF:%8" PRIu64 "|FPS:%8Lf",
                total_elapsed_time,
                (long double) total_elapsed_time / N_uSECONDS_IN_ONE_SEC,
                prev_frame_index,
                next_frame_index_measured_by_time,
                desync,
                frame_timing_sleep,
                total_elapsed_time - last_total_elapsed_time,
                usecs_per_frame,
                current_frame_index / ((long double) total_elapsed_time / N_uSECONDS_IN_ONE_SEC));
        printw("\n%s\n", command_buffer);
        fprintf(logs, "%s\n", command_buffer);
        // =============================================

        prev_frame_index = current_frame_index;
        last_total_elapsed_time = total_elapsed_time;
        total_elapsed_time = get_elapsed_time_from_start_us();
        usecs_per_frame  = total_elapsed_time / current_frame_index + (total_elapsed_time % current_frame_index !=0);
        sleep_time = frame_timing_sleep - (total_elapsed_time % frame_timing_sleep);
        next_frame_index_measured_by_time =  // ceil(total_elapsed_time / frame_timing_sleep)
                total_elapsed_time / frame_timing_sleep + (total_elapsed_time % frame_timing_sleep != 0);
        desync = next_frame_index_measured_by_time - current_frame_index;

        if (reading_type == SOURCE_FILE && desync > 0) {
            for (i=0; i < desync; ++i)
                fread(rgb_data, sizeof(char), TOTAL_READ_SIZE, pipein);
            current_frame_index = next_frame_index_measured_by_time;
        } else if (desync < 0) {
            usleep((current_frame_index - next_frame_index_measured_by_time) * frame_timing_sleep);
        }
        usleep(sleep_time);
    }
    getchar();
    endwin();
    printf("END\n");
    free_space(rgb_data, original_source, pipein, logs);
    return 0;
}
