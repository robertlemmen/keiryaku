#include "global.h"

#include <time.h>

bool arg_debug = false;
bool arg_debug_compiler = false;
bool arg_runtime_stats = false;
int arg_gc_threshold = 100000;
int arg_major_gc_ratio = 4;

long total_gc_time_us = 0;

long currentmicros() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}
