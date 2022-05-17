#ifndef PIX2ASCII_VIDEO_STREAM_H
#define PIX2ASCII_VIDEO_STREAM_H

#define COMMAND_BUFFER_SIZE 512

typedef enum {SOURCE_FILE, SOURCE_CAMERA} source_t;

int get_frame_data(int *frame_width, int *frame_height);
int get_video_stream(source_t reading_type, FILE *video_stream);
int start_player();

#endif //PIX2ASCII_VIDEO_STREAM_H
