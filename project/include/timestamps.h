#ifndef PROJECT_INCLUDE_TIMESTAMPS_H_
#define PROJECT_INCLUDE_TIMESTAMPS_H_

#include <time.h>
#include <inttypes.h>

#define N_uSECONDS_IN_ONE_SEC 1000000

typedef struct timespec timespec;

timespec diff(timespec *start, timespec *end);
uint64_t get_elapsed_time_from_start_us(timespec startTime);

#endif  // PROJECT_INCLUDE_TIMESTAMPS_H_
