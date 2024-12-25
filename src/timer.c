#include "timer.h"
#include <assert.h>

void timer_start(Timer *timer, int type, double timeout) {
    clock_gettime(type /*CLOCK_REALTIME*/, &timer->t0);
    timer->timeout = timeout;
    timer->started = true;
}

void timer_restart(Timer *timer) {
    clock_gettime(timer->type, &timer->t0);
}

void timer_stop(Timer *timer) {
    assert(timer->started);
    clock_gettime(timer->type /*CLOCK_REALTIME*/, &timer->tE);
    timer->delta = (double)(timer->tE.tv_sec - timer->t0.tv_sec) + (double)(timer->tE.tv_nsec - timer->t0.tv_nsec) / 1e9;
}

void timer_continue(Timer *timer) {
    assert(timer->started);
    timer->t0 = timer->tE;
}

bool timer_timeout(Timer *timer) {
    assert(timer->started);
    timer_stop(timer);
    if(timer->delta >= timer->timeout) return true;
    return false;
}


