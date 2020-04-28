#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdbool.h>

extern bool arg_debug;
extern bool arg_debug_compiler;
extern bool arg_runtime_stats;
extern int arg_gc_threshold;
extern int arg_major_gc_ratio;

extern long total_gc_time_us;

long currentmicros();

#endif /* GLOBAL_H */
