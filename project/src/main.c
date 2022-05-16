#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>
#include <limits.h>

#include "frame_utils.h"
#include "timestamps.h"
#include "utils.h"

#define COMMAND_BUFFER_SIZE 512
#define VIDEO_FRAMERATE 25

typedef struct {
    char *char_set;
    int last_index;
} char_set_data;

typedef enum {CHARSET_SHARP, CHARSET_OPTIMAL, CHARSET_STANDART, CHARSET_LONG, CHARSET_N} t_char_set;
static char_set_data char_sets[CHARSET_N] = {
        {"@%#*+=-:. ", 9},
        {"NBUa1|^` ", 8},
        {"N@#W$9876543210?!abc;:+=-,._ ", 28},
        {"$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ", 69}
};

typedef enum {SOURCE_FILE, SOURCE_CAMERA} t_source;

void close_pipe(FILE *pipeline) {
    fflush(pipeline);
    pclose(pipeline);
}

void free_space(unsigned char *video_frame, FILE *or_source, FILE *pipeline, FILE *logs_file){
    free(video_frame);
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

    // flags:
    // -f <Media path>
    // -c : (camera support)
    // -color [sharp | optimal | standard | long] : ascii set
    // -method [average | yuv] : grayscale conversion methods

    frame_full_t frame_full;

    t_source reading_type = SOURCE_FILE;
    char *filepath = NULL;
    t_char_set picked_char_set_type = CHARSET_OPTIMAL;
    frame_full.get_region_intensity = average_chanel_intensity;

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
            } else if (!strcmp(argv[i + 1], "standard")) {
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
                frame_full.get_region_intensity = average_chanel_intensity;
            } else if (!strcmp(argv[i + 1], "yuv")) {
                frame_full.get_region_intensity = yuv_intensity;
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

    frame_full.char_set = char_sets[picked_char_set_type].char_set;
    frame_full.max_char_set_index = char_sets[picked_char_set_type].last_index;

    char command_buffer[COMMAND_BUFFER_SIZE];
    int n_chars_printed = -1;

    int FRAME_WIDTH = 1280, FRAME_HEIGHT = 720;
    // obtaining an interface with ffmpeg
    if (reading_type == SOURCE_CAMERA) {
        n_chars_printed = snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                                   "ffmpeg -hide_banner -loglevel error "
                                   "-f v4l2 -i /dev/video0 -f image2pipe "
                                   "-vf fps=%d -vf scale=%d:%d -vcodec rawvideo -pix_fmt rgb24 -",
                                   VIDEO_FRAMERATE, FRAME_WIDTH, FRAME_HEIGHT);
    } else if (reading_type == SOURCE_FILE) {
        // get input resolution command
        n_chars_printed = snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                                   "ffprobe -v error -select_streams v:0"
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
        if (fscanf(image_data_pipe, "%d %d", &FRAME_WIDTH, &FRAME_HEIGHT) != 2 ||
            fflush(image_data_pipe) || fclose(image_data_pipe)) {
            fprintf(stderr, "Error obtaining input resolution! Width/height not found\n");
            return -1;
        }
        n_chars_printed = snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                                   "ffmpeg -i %s -f image2pipe -hide_banner -loglevel error "
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

    size_t TOTAL_READ_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3;
    frame_full.video_frame = malloc(sizeof(unsigned char) * FRAME_WIDTH * FRAME_HEIGHT * 3);
    // current terminal size in rows and cols
    int n_available_rows = 0, n_available_cols = 0;
    int new_n_available_rows, new_n_available_cols;

    // video frame downsample coefficients
    frame_full.row_downscale_coef = 1;
    frame_full.col_downscale_coef = 1;

    size_t n_read_items;  // n bytes read from pipe
    size_t prev_frame_index = 0, current_frame_index = 0;
    size_t next_frame_index_measured_by_time = 0;
    long long int desync=0, i;

    size_t total_elapsed_time, last_total_elapsed_time=0;
    // !size_t int division doesn't work with operand of other type!
    size_t frame_timing_sleep = N_uSECONDS_IN_ONE_SEC / VIDEO_FRAMERATE;
    size_t usecs_per_frame=0, sleep_time;

    FILE *logs = fopen("Logs.txt", "w");

    timespec startTime;
    // ==================== aligning player and program output ====================
    // We force ffplay (our video player) to write its log to a file called <StartIndicator>.
    // It looks like this:

    // ffplay started on 2022-05-06 at 21:39:01
    //Report written to "StartIndicator"
    //Log level: 32
    //Command line:
    //ffplay <filepath> -hide_banner -loglevel error -nostats -vf showinfo
    //[Parsed_showinfo_0 @ 0x7fc458002f00] config in time_base: 1/1000, frame_rate: 30/1
    //[Parsed_showinfo_0 @ 0x7fc458002f00] config out time_base: 0/0, frame_rate: 0/0
    //[Parsed_showinfo_0 @ 0x7fc458002f00] n:   0 pts:      0 pts_time:0       pos:      ...   <------ The line we
    //  ...                                                                                            are looking for
    //

    // When ffplay starts to process video frames it logs it in the THIRD line
    // that starts with symbol '['. That symbol we are constantly looking for.

    FILE *original_source = NULL;
    if (reading_type == SOURCE_FILE) {
        FILE *ffplay_log_file = fopen("StartIndicator", "w");
        assert(ffplay_log_file);
        fclose(ffplay_log_file);
        snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                 "FFREPORT=file=StartIndicator:level=32 "
                 "ffplay %s -hide_banner -loglevel error -nostats -vf showinfo -framedrop", filepath);
        original_source = popen(command_buffer, "r");

        ffplay_log_file = fopen("StartIndicator", "r");
        int n_bracket_encounters = 0;
        while (n_bracket_encounters < 3) {
            char current_file_char;
            if ((current_file_char = getc(ffplay_log_file)) == EOF) {
                long int current_file_position = ftell(ffplay_log_file)-1;
                fclose(ffplay_log_file);
                ffplay_log_file = fopen("StartIndicator", "r");
                fseek(ffplay_log_file, current_file_position, SEEK_SET);
            }
            if (current_file_char == '[')
                ++n_bracket_encounters;
        }
        fclose(ffplay_log_file);
    }
    // ================================================================================

    clock_gettime(CLOCK_MONOTONIC_COARSE, &startTime);
    initscr();
    curs_set(0);
    total_elapsed_time = get_elapsed_time_from_start_us(startTime);
    while ((n_read_items = fread(frame_full.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein)) || !feof(pipein)) {
        if (n_read_items < TOTAL_READ_SIZE) {
            total_elapsed_time = get_elapsed_time_from_start_us(startTime);
            sleep_time = frame_timing_sleep - (total_elapsed_time % frame_timing_sleep);
            usleep(sleep_time);
            continue;
        }
        ++current_frame_index;  // current_frame_index is incremented because of fread()

        // terminal resize check
        getmaxyx(stdscr, new_n_available_rows, new_n_available_cols);
        if (n_available_rows != new_n_available_rows ||
            n_available_cols != new_n_available_cols) {
            n_available_rows = new_n_available_rows;
            n_available_cols = new_n_available_cols;
            frame_full.row_downscale_coef = MY_MAX((FRAME_HEIGHT + n_available_rows) / n_available_rows, 1);
            frame_full.col_downscale_coef = MY_MAX((FRAME_WIDTH  + n_available_cols) / n_available_cols, 1);

            frame_full.trimmed_height = FRAME_HEIGHT - FRAME_HEIGHT % frame_full.row_downscale_coef;
            frame_full.trimmed_width = FRAME_WIDTH - FRAME_WIDTH % frame_full.col_downscale_coef;
            frame_full.left_border_indent = (n_available_cols - frame_full.trimmed_width / frame_full.col_downscale_coef) / 2;
            clear();
        }
        // ASCII frame preparation
        frame_full.frame_width = FRAME_WIDTH;
        frame_full.frame_width = FRAME_WIDTH;
        frame_full.frame_width = FRAME_WIDTH;
        
        draw_frame(frame_full);
        // ASCII frame drawing
        refresh();

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

        // "EL uS:%10llu|EL S:%8.2f|FI:%5llu|TFI:%5llu|TFI - FI:%2d|uSPF:%8llu|Cur uSPF:%8llu|Avg uSPF:%8llu|FPS:%8f"
        snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                 "EL uS:%10" PRIu64 "|EL S:%8.2f|FI:%5" PRIu64 "|TFI:%5" PRIu64 "|TFI - FI:%2" PRId64
                 "|uSPF:%8" PRIu64 "|Cur uSPF:%8" PRIu64 "|Avg uSPF:%8" PRIu64 "|FPS:%8f",
                 total_elapsed_time,
                 (double) total_elapsed_time / N_uSECONDS_IN_ONE_SEC,
                 prev_frame_index,
                 next_frame_index_measured_by_time,
                 (long int) desync,
                 frame_timing_sleep,
                 total_elapsed_time - last_total_elapsed_time,
                 usecs_per_frame,
                 prev_frame_index / ((double) total_elapsed_time / N_uSECONDS_IN_ONE_SEC));
        printw("\n%s\n", command_buffer);
        fprintf(logs, "%s\n", command_buffer);
        // =============================================
        last_total_elapsed_time = total_elapsed_time;
        total_elapsed_time = get_elapsed_time_from_start_us(startTime);
        usecs_per_frame  = total_elapsed_time / current_frame_index + (total_elapsed_time % current_frame_index != 0);
        sleep_time = frame_timing_sleep - (total_elapsed_time % frame_timing_sleep);
        next_frame_index_measured_by_time =  // ceil(total_elapsed_time / frame_timing_sleep)
                total_elapsed_time / frame_timing_sleep + (total_elapsed_time % frame_timing_sleep != 0);
        desync = next_frame_index_measured_by_time - current_frame_index;

        prev_frame_index = current_frame_index;
        if (reading_type == SOURCE_FILE && desync > 0) {
            for (i=0; i < desync; ++i)
                fread(frame_full.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein);
            current_frame_index = next_frame_index_measured_by_time;
        } else if (desync < 0) {
            usleep((current_frame_index - next_frame_index_measured_by_time) * frame_timing_sleep);
        }
        usleep(sleep_time);
    }
    getchar();
    endwin();
    printf("END\n");
    free_space(frame_full.video_frame, original_source, pipein, logs);
    return 0;
}
