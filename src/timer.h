#include <math.h>
#ifndef TIMER_H

#include <time.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct Timer {
    struct timespec t0;
    struct timespec tE;
    pthread_t thread;
    double timeout;
    double delta;
    int type;
    bool started;
    bool timedout;
} Timer;

int timespec_geq(struct timespec a, struct timespec b);
void timespec_add(struct timespec *a, struct timespec *b, double t);

void timer_start(Timer *timer, int type, double timeout);
void timer_restart(Timer *timer);
void timer_stop(Timer *timer);
void timer_continue(Timer *timer);
bool timer_timedout(Timer *timer);

#define TIMER_H
#endif

