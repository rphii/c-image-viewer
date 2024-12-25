#include "timer.h"

Timer timer_start(int type) {
    Timer result;
    clock_gettime(type /*CLOCK_REALTIME*/, &result.t0);
}

double timer_delta(Timer timer) {
    clock_gettime(timer.type /*CLOCK_REALTIME*/, &timer.tE);
    return (double)(timer.tE.tv_sec - timer.t0.tv_sec) + (double)(timer.tE.tv_nsec - timer.t0.tv_nsec) / 1e9;
}


