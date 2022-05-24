#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>

#include "videostream.h"
#include "frame_processing.h"
#include "timestamps.h"
#include "argparsing.h"
#include "termstream.h"


static void close_pipe(FILE *pipeline) {
    fflush(pipeline);
    pclose(pipeline);
}

static void free_space(unsigned char *video_frame, FILE *pipeline, FILE *logs_file){
    free(video_frame);
    close_pipe(pipeline);
    fflush(logs_file);
    fclose(logs_file);
}

int main(int argc, char *argv[]) {
    user_params_t user_params;
    if (argparse(&user_params, argc, argv)) {
        // ...
        return 1;
    }

    FILE *pipein = NULL;
    frame_params_t frame_data;

    frame_data.width = 1280;
    frame_data.height = 720;
    if (user_params.reading_type == SOURCE_FILE) {
        if (!(pipein = get_file_stream(user_params.file_path, user_params.n_stream_loops))) {
            // ...
            return 1;
        }
        if (get_frame_data(user_params.file_path, &frame_data.width, &frame_data.height)) {
            // ...
            return 1;
        }
        if (start_player()) {
            // ...
            return 1;
        }
    } else if (user_params.reading_type == SOURCE_CAMERA) {
        if (!(pipein = get_camera_stream(frame_data.width, frame_data.height))) {
            // ...
            return 1;
        }
    } else {
        // ...
        return 1;
    }

    int TOTAL_READ_SIZE = frame_data.width * frame_data.height * 3;
    frame_data.video_frame = malloc(sizeof(unsigned char) * TOTAL_READ_SIZE);
    if (!frame_data.video_frame) {
        // ...
        return 1;
    }

    debug_info_t current_frame_info;
    current_frame_info.uS_elapsed = 0;
    current_frame_info.frame_index = 0;
    current_frame_info.time_frame_index = 0;
    current_frame_info.frame_desync = 0;

    size_t prev_uS_elapsed, sleep_time;
    size_t frame_timing_sleep = N_uSECONDS_IN_ONE_SEC / VIDEO_FRAMERATE;

    unsigned long n_read_items;  // n bytes read from pipe
    FILE *logs = fopen("Logs.txt", "w");

    timespec startTime;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &startTime);
    initscr();
    draw_frame_t draw_frame = (user_params.color_flag) ? (draw_color_frame) : (draw_symbol_frame);
    start_color();
    set_color_pairs();
    curs_set(0);
    current_frame_info.uS_elapsed = get_elapsed_time_from_start_us(startTime);
    while ((n_read_items = fread(frame_data.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein)) || !feof(pipein)) {
        if (n_read_items < TOTAL_READ_SIZE) {
            current_frame_info.uS_elapsed = get_elapsed_time_from_start_us(startTime);
            sleep_time = frame_timing_sleep - (current_frame_info.uS_elapsed % frame_timing_sleep);
            usleep(sleep_time);
            continue;
        }
        ++current_frame_info.frame_index;  // current_frame_index is incremented because of fread()

        // ASCII frame preparation
        // draw_frame(&frame_data, user_params.charset_data.char_set, user_params.charset_data.last_index,
        //            user_params.pixel_block_processing_method);
        draw_frame(&frame_data, user_params.charset_data.char_set, user_params.charset_data.last_index,
                         user_params.pixel_block_processing_method);
        debug(&current_frame_info, logs);
        // ASCII frame drawing
        refresh();
        prev_uS_elapsed = current_frame_info.uS_elapsed;
        current_frame_info.uS_elapsed = get_elapsed_time_from_start_us(startTime);
        current_frame_info.cur_frame_processing_time = current_frame_info.uS_elapsed - prev_uS_elapsed;

        sleep_time = frame_timing_sleep - (current_frame_info.uS_elapsed % frame_timing_sleep);
        current_frame_info.time_frame_index =  // ceil(total_elapsed_time / frame_timing_sleep)
                current_frame_info.uS_elapsed / frame_timing_sleep +
                (current_frame_info.uS_elapsed % frame_timing_sleep != 0);

        current_frame_info.frame_desync = (current_frame_info.time_frame_index > current_frame_info.frame_index)
                ? current_frame_info.time_frame_index - current_frame_info.frame_index
                : current_frame_info.frame_index - current_frame_info.time_frame_index;

        if (user_params.reading_type == SOURCE_FILE && current_frame_info.time_frame_index > current_frame_info.frame_index) {
            for (size_t i=0; i<current_frame_info.frame_desync; ++i)
                fread(frame_data.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein);
            current_frame_info.frame_index = current_frame_info.time_frame_index;
        } else if (current_frame_info.time_frame_index < current_frame_info.frame_index) {
            usleep((current_frame_info.frame_index - current_frame_info.time_frame_index) * frame_timing_sleep);
        }
        usleep(sleep_time);
    }
    getchar();
    endwin();
    printf("END\n");
    free_space(frame_data.video_frame, pipein, logs);
    return 0;
}
