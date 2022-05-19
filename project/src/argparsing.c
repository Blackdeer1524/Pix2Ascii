#include "../include/argparsing.h"


int argparse(user_params_t *user_params, int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Bad number of arguments!\n");
        return -1;
    }
    // flags:
    // -f <Media path>
    // -c : (camera support)
    // -color [sharp | optimal | standard | long] : ascii set
    // -method [average | yuv] : grayscale conversion methods
    //    t_source reading_type = SOURCE_FILE;
//    char *filepath = NULL;
//    t_char_set picked_char_set_type = CHARSET_OPTIMAL;
//    region_intensity_t grayscale_method = average_chanel_intensity;
//
//    // argument parsing
//    for (int i=1; i<argc;) {
//        if (argv[i][0] != '-') {
//            fprintf(stderr, "Invalid argument! Value is given without a corresponding flag!\n");
//            return -1;
//        }
//
//        if (!strcmp(&argv[i][1], "c")) {
//            reading_type = SOURCE_CAMERA;
//            ++i;
//        } else if (!strcmp(&argv[i][1], "f")) {
//            if (i == argc - 1 || argv[i + 1][0] == '-') {
//                fprintf(stderr, "Invalid argument! Filepath is not given!\n");
//                return -1;
//            } else {
//                reading_type = SOURCE_FILE;
//                filepath = argv[i + 1];
//                i += 2;
//            }
//        } else if (!strcmp(&argv[i][1], "color")) {
//            if (i == argc - 1 || argv[i + 1][0] == '-') {
//                fprintf(stderr, "Invalid argument! Color scheme is not given!\n");
//                return -1;
//            }
//
//            if (!strcmp(argv[i + 1], "sharp")) {
//                picked_char_set_type = CHARSET_SHARP;
//            } else if (!strcmp(argv[i + 1], "optimal")) {
//                picked_char_set_type = CHARSET_OPTIMAL;
//            } else if (!strcmp(argv[i + 1], "standard")) {
//                picked_char_set_type = CHARSET_STANDART;
//            } else if (!strcmp(argv[i + 1], "long")) {
//                picked_char_set_type = CHARSET_LONG;
//            } else {
//                fprintf(stderr, "Invalid argument! Unsupported scheme!\n");
//                return -1;
//            }
//            i += 2;
//        } else if (!strcmp(&argv[i][1], "method")) {
//            if (i == argc - 1 || argv[i + 1][0] == '-') {
//                fprintf(stderr, "Invalid argument! Color scheme is not given!\n");
//                return -1;
//            }
//
//            if (!strcmp(argv[i + 1], "average")) {
//                grayscale_method = average_chanel_intensity;
//            } else if (!strcmp(argv[i + 1], "yuv")) {
//                grayscale_method = yuv_intensity;
//            } else {
//                fprintf(stderr, "Invalid argument! Unsupported grayscale method!\n");
//                return -1;
//            }
//            i += 2;
//        } else {
//            fprintf(stderr, "Unknown flag!\n");
//            return -1;
//        }
//    }
    return 0;
}
