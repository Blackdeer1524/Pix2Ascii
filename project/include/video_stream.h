#ifndef PIX2ASCII_VIDEO_STREAM_H
#define PIX2ASCII_VIDEO_STREAM_H

int get_frame_data(int *frame_width, int *frame_height);
int get_video_stream(int reading_type, FILE *video_stream);
int start_player();

#endif //PIX2ASCII_VIDEO_STREAM_H
