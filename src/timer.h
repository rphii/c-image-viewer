#include <math.h>
#ifndef TIMER_H

#include <time.h>
#include <stdbool.h>

typedef struct Timer {
    struct timespec t0;
    struct timespec tE;
    double timeout;
    double delta;
    int type;
    bool started;
} Timer;

void timer_start(Timer *timer, int type, double timeout);
void timer_restart(Timer *timer);
void timer_stop(Timer *timer);
void timer_continue(Timer *timer);
bool timer_timeout(Timer *timer);

#define TIMER_H
#endif

