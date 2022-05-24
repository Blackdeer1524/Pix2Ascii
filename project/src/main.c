#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "../include/video_stream.h"
#include "../include/frame_utils.h"
#include "../include/timestamps.h"
#include "../include/argparsing.h"
#include "../include/error.h"


typedef struct {
    char *char_set;
    unsigned int last_index;
} char_set_data;

typedef enum {
    CHARSET_SHARP,
    CHARSET_OPTIMAL,
    CHARSET_STANDART,
    CHARSET_LONG,
    CHARSET_N
} t_char_set;

static char_set_data char_sets[CHARSET_N] = {
        {"@%#*+=-:. ", 9},
        {"NBUa1|^` ", 8},
        {"N@#W$9876543210?!abc;:+=-,._ ", 28},
        {"$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ", 69}
};


void close_pipe(FILE *pipeline) {
    fflush(pipeline);
    pclose(pipeline);
}

void free_space(unsigned char *video_frame, FILE *pipeline, FILE *logs_file){
    free(video_frame);
    close_pipe(pipeline);
    fflush(logs_file);
    fclose(logs_file);
}

int main(int argc, char *argv[]) {
    user_params_t user_params;

    if (arg_parse(&user_params, argc, argv)) {
        // ...
        return ARG_COUNT_ERROR;
    }

    user_params.charset_data = "N@#W$9876543210?!abc;:+=-,._ ";
    user_params.pixel_block_processing_method = average_chanel_intensity;

    FILE *pipein = NULL;
    frame_data_t frame_data = {NULL, 1280, 720};
    if (user_params.reading_type == SOURCE_FILE) {
        if (!(pipein = get_file_stream(user_params.file_path))) {
            // ...
            return -1;
        }
        if (get_frame_data(user_params.file_path, &frame_data.width, &frame_data.height)) {
            // ...
            return -1;
        }
        if (start_player(user_params.file_path)) {
            // ...
            return -1;
        }
    } else if (user_params.reading_type == SOURCE_CAMERA) {
        if (!(pipein = get_camera_stream(frame_data.width, frame_data.height))) {
            // ...
            return -1;
        }
    } else {
        // ...
        return -1;
    }
    assert(pipein);
    int TOTAL_READ_SIZE = frame_data.width * frame_data.height * 3;
    frame_data.video_frame = malloc(sizeof(unsigned char) * TOTAL_READ_SIZE);
    // current terminal size in rows and cols
    int n_available_rows = 0, n_available_cols = 0;
    int new_n_available_rows, new_n_available_cols;

    // video frame downsample coefficients
    int row_downscale_coef = 1, col_downscale_coef = 1;
    int trimmed_height, trimmed_width;
    int left_border_indent;

    int n_read_items;  // n bytes read from pipe
    size_t prev_frame_index = 0, current_frame_index = 0;
    size_t next_frame_index_measured_by_time = 0;
    size_t desync = 0, i;

    size_t total_elapsed_time, last_total_elapsed_time=0;
    // !uint64_t int division doesn't work with operand of other type!
    size_t frame_timing_sleep = N_uSECONDS_IN_ONE_SEC / VIDEO_FRAMERATE;
    size_t usecs_per_frame=0, sleep_time;

    FILE *logs = fopen("Logs.txt", "w");

    timespec startTime;

    clock_gettime(CLOCK_MONOTONIC_COARSE, &startTime);
    initscr();
    curs_set(0);
    total_elapsed_time = get_elapsed_time_from_start_us(startTime);
    while ((n_read_items = fread(frame_data.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein)) || !feof(pipein)) {
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
            row_downscale_coef = MAX((frame_data.height + n_available_rows) / n_available_rows, 1);
            col_downscale_coef = MAX((frame_data.width  + n_available_cols) / n_available_cols, 1);

            trimmed_height = frame_data.height - frame_data.height % row_downscale_coef;
            trimmed_width = frame_data.width - frame_data.width % col_downscale_coef;
            left_border_indent = (n_available_cols - trimmed_width / col_downscale_coef) / 2;
            clear();
        }
        // ASCII frame preparation
        draw_frame(frame_data.video_frame, frame_data.width, trimmed_height, trimmed_width, row_downscale_coef, col_downscale_coef,
                   left_border_indent, user_params.charset_data, strlen(user_params.charset_data)-1, user_params.pixel_block_processing_method);
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
//        snprintf(command_buffer, COMMAND_BUFFER_SIZE,
//                 "EL uS:%10zu|EL S:%8.2f|FI:%5zu|TFI:%5zu|abs(TFI - FI):%2zu|uSPF:%8d|Cur uSPF:%8zu|Avg uSPF:%8zu|FPS:%8f",
//                 total_elapsed_time,
//                 (double) total_elapsed_time / N_uSECONDS_IN_ONE_SEC,
//                 prev_frame_index,
//                 next_frame_index_measured_by_time,
//                 desync,
//                 frame_timing_sleep,
//                 total_elapsed_time - last_total_elapsed_time,
//                 usecs_per_frame,
//                 prev_frame_index / ((double) total_elapsed_time / N_uSECONDS_IN_ONE_SEC));
//        printw("\n%s\n", command_buffer);
//        fprintf(logs, "%s\n", command_buffer);
        // =============================================
        prev_frame_index = current_frame_index;
        last_total_elapsed_time = total_elapsed_time;
        total_elapsed_time = get_elapsed_time_from_start_us(startTime);
        usecs_per_frame  = total_elapsed_time / current_frame_index + (total_elapsed_time % current_frame_index != 0);
        sleep_time = frame_timing_sleep - (total_elapsed_time % frame_timing_sleep);
        next_frame_index_measured_by_time =  // ceil(total_elapsed_time / frame_timing_sleep)
                total_elapsed_time / frame_timing_sleep + (total_elapsed_time % frame_timing_sleep != 0);

        desync = (next_frame_index_measured_by_time > current_frame_index)
                ? next_frame_index_measured_by_time - current_frame_index
                : current_frame_index - next_frame_index_measured_by_time;

        if (user_params.reading_type == SOURCE_FILE && next_frame_index_measured_by_time > current_frame_index) {
            fread(frame_data.video_frame, sizeof(char), TOTAL_READ_SIZE * desync, pipein);
            current_frame_index = next_frame_index_measured_by_time;
        } else if (next_frame_index_measured_by_time < current_frame_index) {
            usleep((current_frame_index - next_frame_index_measured_by_time) * frame_timing_sleep);
        }
        usleep(sleep_time);
    }
    getchar();
    endwin();
    printf("END\n");
    free_space(frame_data.video_frame, pipein, logs);
    return 0;
}
