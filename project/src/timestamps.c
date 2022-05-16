#include "timestamps.h"


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
size_t get_elapsed_time_from_start_us(timespec startTime) {
    static timespec tmpTime, diffTime;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &tmpTime);
    diffTime = diff(&startTime, &tmpTime);
    return  ((size_t) diffTime.tv_sec * N_uSECONDS_IN_ONE_SEC * 1000 + (size_t) diffTime.tv_nsec) / 1000;
}
