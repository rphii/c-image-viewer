#include "timer.h"
#include <assert.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

int timespec_geq(struct timespec a, struct timespec b) {
    if(a.tv_sec == b.tv_sec) {
        return a.tv_nsec >= b.tv_nsec;
    }
    return a.tv_sec >= b.tv_sec;
}

void timespec_add(struct timespec *a, struct timespec *b, double t) {
    time_t s = round(t);
    time_t ns = round((t - s) * 1e9);
    struct timespec temp = *a;
    temp.tv_sec += s;
    temp.tv_nsec += ns;
    temp.tv_sec += temp.tv_nsec / 1000000000;
    temp.tv_nsec %= 1000000000;
    *b = temp;
}

void *timer_idle(void *argp) {
    Timer *timer = argp;
    //printf("SLEEPING ...\n");
    long n = 0;
sleep:
    clock_nanosleep(timer->type, TIMER_ABSTIME, &timer->tE, 0);
    struct timespec tN;
    struct timespec tE = timer->tE; /* re-fetch tE */
    clock_gettime(timer->type, &tN);
    //printf("TIMED OUT! %zu : %zus %zuns / %zus %zuns\n", ++n, tN.tv_sec, tN.tv_nsec, tE.tv_sec, tE.tv_nsec);
    if(timer->started && timespec_geq(tN, tE)) {
        timer->timedout = true;
        timer->started = false;
        glfwPostEmptyEvent();
        return 0;
    }
    goto sleep;
}

void timer_start(Timer *timer, int type, double timeout) {
    clock_gettime(type, &timer->t0);
    timer->timeout = timeout;
    if(timeout) {
        bool started = timer->started;
        timer->started = false;
        /* start background thread */
        timespec_add(&timer->t0, &timer->tE, timeout);
        timer->timedout = false;
        timer->started = true;
        if(timer->timeout && started) return;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&timer->thread, &attr, timer_idle, timer);
    } else {
        timer->started = true;
    }
}

void timer_restart(Timer *timer) {
    assert(timer->started);
    timer_start(timer, timer->type, timer->timeout);
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

bool timer_timedout(Timer *timer) {
#if 0
    assert(timer->started);
    timer_stop(timer);
    if(timer->delta >= timer->timeout) return true;
    return false;
#endif
    bool result = timer->timedout;
    if(result) {
        timer->timedout = false;
    }
    return result;
}


