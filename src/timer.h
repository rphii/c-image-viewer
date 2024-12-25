#include <math.h>
#ifndef TIMER_H

#include <time.h>

typedef struct Timer {
    struct timespec t0;
    struct timespec tE;
    int type;
} Timer;

Timer timer_start(int type);
double timer_delta(Timer timer);

#define TIMER_H
#endif

