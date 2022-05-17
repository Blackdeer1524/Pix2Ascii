//
// Created by blackdeer on 5/17/22.
//
#include <stdio.h>

#include "../include/video_stream.h"

#define COMMAND_BUFFER_SIZE 512
static char command_buffer[COMMAND_BUFFER_SIZE];

int get_frame_data(const char *filepath, int *frame_width, int *frame_height) {
    int n_chars_printed = snprintf(command_buffer, COMMAND_BUFFER_SIZE,
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
    if (fscanf(image_data_pipe, "%d %d", frame_width, frame_height) != 2 ||
    fflush(image_data_pipe) || fclose(image_data_pipe)) {
        fprintf(stderr, "Error obtaining input resolution! Width/height not found\n");
        return -1;
    }
    return 0;
}


FILE *get_camera_stream(int frame_width, int frame_height) {
    int n_chars_printed = snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                               "ffmpeg -hide_banner -loglevel error "
                               "-f v4l2 -i /dev/video0 -f image2pipe "
                               "-vf fps=%d -vf scale=%u:%u -vcodec rawvideo -pix_fmt rgb24 -",
                               VIDEO_FRAMERATE, frame_width, frame_height);
    if (n_chars_printed < 0) {
        fprintf(stderr, "Error setting up camera!\n");
        return NULL;
    } else if (n_chars_printed >= COMMAND_BUFFER_SIZE) {
        fprintf(stderr, "Error setting up camera! Query size is too big!\n");
        return NULL;
    }
    // sets up stream from where we read our RGB frames
    FILE *video_stream = popen(command_buffer, "r");
    if (!video_stream) {
        fprintf(stderr, "Error setting up camera! Couldn't set an interface with camera!\n");
        return NULL;
    }
    return video_stream;
}


FILE *get_file_stream(const char *file_path) {
    int n_chars_printed = snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                               "ffmpeg -i %s -f image2pipe -hide_banner -loglevel error "
                               "-vf fps=%d -vcodec rawvideo -pix_fmt rgb24 -",
                               file_path, VIDEO_FRAMERATE);
    if (n_chars_printed < 0) {
        fprintf(stderr, "Error preparing ffmpeg command!\n");
        return NULL;
    } else if (n_chars_printed >= COMMAND_BUFFER_SIZE) {
        fprintf(stderr, "Error preparing ffmpeg command! Query size is too big!\n");
        return NULL;
    }
    // sets up stream from where we read our RGB frames
    FILE *video_stream = popen(command_buffer, "r");
    if (!video_stream) {
        fprintf(stderr, "Error obtaining data stream! Couldn't set an interface with ffmpeg!\n");
        return NULL;
    }
    return video_stream;
}

int start_player() {
    FILE *original_source = NULL;
    if (reading_type == SOURCE_FILE) {
        FILE *ffplay_log_file = fopen("StartIndicator", "w");
        assert(ffplay_log_file);
        fclose(ffplay_log_file);
        snprintf(command_buffer, COMMAND_BUFFER_SIZE,
                 "FFREPORT=file=StartIndicator:level=32 "
                 "ffplay %s -hide_banner -loglevel error -nostats -vf showinfo -framedrop", filepath);
        // Через fork() и exec (?)
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

    return 0;
}
