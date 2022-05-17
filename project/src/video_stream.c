//
// Created by blackdeer on 5/17/22.
//

#include "video_stream.h"
int get_frame_data(int *frame_width, int *frame_height) {
    // ...
    return 0;
}


int get_video_stream(int reading_type, FILE *video_stream) {
    // ...
    return 0;
}

int start_player() {
//    // ==================== aligning player and program output ====================
//    // We force ffplay (our video player) to write its log to a file called <StartIndicator>.
//    // It looks like this:
//
//    // ffplay started on 2022-05-06 at 21:39:01
//    //Report written to "StartIndicator"
//    //Log level: 32
//    //Command line:
//    //ffplay <filepath> -hide_banner -loglevel error -nostats -vf showinfo
//    //[Parsed_showinfo_0 @ 0x7fc458002f00] config in time_base: 1/1000, frame_rate: 30/1
//    //[Parsed_showinfo_0 @ 0x7fc458002f00] config out time_base: 0/0, frame_rate: 0/0
//    //[Parsed_showinfo_0 @ 0x7fc458002f00] n:   0 pts:      0 pts_time:0       pos:      ...   <------ The line we
//    //  ...                                                                                            are looking for
//    //
//
//    // When ffplay starts to process video frames it logs it in the THIRD line
//    // that starts with symbol '['. That symbol we are constantly looking for.
//
//    FILE *original_source = NULL;
//    if (reading_type == SOURCE_FILE) {
//        FILE *ffplay_log_file = fopen("StartIndicator", "w");
//        assert(ffplay_log_file);
//        fclose(ffplay_log_file);
//        snprintf(command_buffer, COMMAND_BUFFER_SIZE,
//                 "FFREPORT=file=StartIndicator:level=32 "
//                 "ffplay %s -hide_banner -loglevel error -nostats -vf showinfo -framedrop", filepath);
//        original_source = popen(command_buffer, "r");
//
//        ffplay_log_file = fopen("StartIndicator", "r");
//        int n_bracket_encounters = 0;
//        while (n_bracket_encounters < 3) {
//            char current_file_char;
//            if ((current_file_char = getc(ffplay_log_file)) == EOF) {
//                long int current_file_position = ftell(ffplay_log_file)-1;
//                fclose(ffplay_log_file);
//                ffplay_log_file = fopen("StartIndicator", "r");
//                fseek(ffplay_log_file, current_file_position, SEEK_SET);
//            }
//            if (current_file_char == '[')
//                ++n_bracket_encounters;
//        }
//        fclose(ffplay_log_file);
//    }
//    // ================================================================================
    return 0;
}