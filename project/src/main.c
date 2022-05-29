#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>

#include "videostream.h"
#include "frame_processing.h"
#include "timestamps.h"
#include "argparsing.h"
#include "termstream.h"
#include "status_codes.h"


static void close_pipe(FILE *pipeline) {
    fflush(pipeline);
    pclose(pipeline);
}

static void free_space(unsigned char *video_frame, FILE *pipeline, FILE *logs_file) {
    free(video_frame);
    close_pipe(pipeline);
    fflush(logs_file);
    fclose(logs_file);
}

int main(int argc, char *argv[]) {
    user_params_t user_params;
    int return_status;
    if ((return_status = argparse(&user_params, argc, argv))) {
        if (return_status == HELP_FLAG)
            return SUCCESS;
        return return_status;
    }

    FILE *pipein = NULL;
    frame_params_t frame_data;

    frame_data.width = 1280;
    frame_data.height = 720;
    if (user_params.ffmpeg_params.reading_type == SOURCE_FILE) {
        if (!(pipein = get_file_stream(user_params.ffmpeg_params.file_path, user_params.ffmpeg_params.n_stream_loops))) {
            // ...
            return POPEN_ERROR;
        }
        if ((return_status = get_frame_data(user_params.ffmpeg_params.file_path, &frame_data.width, &frame_data.height))) {
            // ...
            return return_status;
        }
        if ((return_status = start_player(user_params.ffmpeg_params.file_path,
                                          user_params.ffmpeg_params.n_stream_loops + 1,
                                          user_params.ffmpeg_params.player_flag))) {
            // ...
            return return_status;
        }
    } else if (user_params.ffmpeg_params.reading_type == SOURCE_CAMERA) {
        if (!(pipein = get_camera_stream(frame_data.width, frame_data.height))) {
            // ...
            return POPEN_ERROR;
        }
    } else {
        fprintf(stderr, "Unknown source format!");
        return NOT_IMPLEMENTED_ERROR;
    }

    timespec startTime;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &startTime);

    FILE *logs = fopen("Logs.txt", "w");
    if (!logs) {
        fprintf(stderr, "Couldn't open log file!");
        return FOPEN_ERROR;
    }

    frame_data.triple_width = frame_data.width * 3;
    int TOTAL_READ_SIZE = frame_data.triple_width * frame_data.height;
    frame_data.video_frame = malloc(sizeof(unsigned char) * TOTAL_READ_SIZE);
    if (!frame_data.video_frame) {
        fprintf(stderr, "Couldn't allocate memory for frame!");
        return_status = FRAME_ALLOCATION_ERROR;
        goto free_memory;
    }

    sync_info_t frame_sync_info;
    frame_sync_info.uS_elapsed = 0;
    frame_sync_info.frame_index = 0;
    frame_sync_info.time_frame_index = 0;
    frame_sync_info.frame_desync = 0;
    size_t prev_uS_elapsed, sleep_time;
    size_t frame_timing_sleep = N_uSECONDS_IN_ONE_SEC / VIDEO_FRAMERATE;

    kernel_params_t kernel_data;
    kernel_data.kernel = NULL;
    kernel_data.update_kernel = user_params.frame_processing_params.update_kernel;
    int left_border_indent;

    initscr();
    curs_set(0);
    display_method_t symbol_display_method;
    if (user_params.terminal_params.color_flag) {
        start_color();
        set_color_pairs();
        symbol_display_method = colored_display;
    } else {
        symbol_display_method = simple_display;
    }

    unsigned long n_read_items;  // n bytes read from pipe
    frame_sync_info.uS_elapsed = get_elapsed_time_from_start_us(startTime);
    while ((n_read_items = fread(frame_data.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein)) || !feof(pipein)) {
        if (n_read_items < TOTAL_READ_SIZE) {
            frame_sync_info.uS_elapsed = get_elapsed_time_from_start_us(startTime);
            sleep_time = frame_timing_sleep - (frame_sync_info.uS_elapsed % frame_timing_sleep);
            usleep(sleep_time);
            continue;
        }
        ++frame_sync_info.frame_index;  // current_frame_index is incremented because of fread()

        // ASCII frame preparation
        // draw_frame(&frame_data, user_params.charset_params.char_set, user_params.charset_params.last_index,
        //            user_params.frame_processing_params.rgb_channels_processor);
        if ((return_status = update_terminal_size(&frame_data, &kernel_data,
                                                  user_params.terminal_params, &left_border_indent)))
            goto free_memory;

        draw_frame(&frame_data, &kernel_data, user_params.charset_params, left_border_indent,
                   user_params.frame_processing_params.rgb_channels_processor, symbol_display_method);
        debug(&frame_sync_info, logs, symbol_display_method);
        // ASCII frame drawing
        refresh();
        prev_uS_elapsed = frame_sync_info.uS_elapsed;
        frame_sync_info.uS_elapsed = get_elapsed_time_from_start_us(startTime);
        frame_sync_info.cur_frame_processing_time = frame_sync_info.uS_elapsed - prev_uS_elapsed;

        sleep_time = frame_timing_sleep - (frame_sync_info.uS_elapsed % frame_timing_sleep);
        frame_sync_info.time_frame_index =  // ceil(total_elapsed_time / frame_timing_sleep)
                frame_sync_info.uS_elapsed / frame_timing_sleep +
                (frame_sync_info.uS_elapsed % frame_timing_sleep != 0);

        frame_sync_info.frame_desync = (frame_sync_info.time_frame_index > frame_sync_info.frame_index)
                ? frame_sync_info.time_frame_index - frame_sync_info.frame_index
                : frame_sync_info.frame_index - frame_sync_info.time_frame_index;

        if (user_params.ffmpeg_params.reading_type == SOURCE_FILE && frame_sync_info.time_frame_index > frame_sync_info.frame_index) {
            for (size_t i=0; i<frame_sync_info.frame_desync; ++i)
                fread(frame_data.video_frame, sizeof(char), TOTAL_READ_SIZE, pipein);
            frame_sync_info.frame_index = frame_sync_info.time_frame_index;
        } else if (frame_sync_info.time_frame_index < frame_sync_info.frame_index) {
            usleep((frame_sync_info.frame_index - frame_sync_info.time_frame_index) * frame_timing_sleep);
        }
        usleep(sleep_time);
    }
    free_memory:
        getchar();
        endwin();
        printf("END\n");
        free_space(frame_data.video_frame, pipein, logs);
    return return_status;
}
