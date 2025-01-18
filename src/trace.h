#ifndef TRACE_H

#include "err.h"
#include "str.h"

#define TRACE_DEPTH 32

typedef struct Trace {
    void *array[TRACE_DEPTH];
    char **strings;
    unsigned int length;
} Trace;

#define ERR_trace_grab(...) "failed grabbing a trace"
ErrDecl trace_grab(Trace *trace);
#define ERR_trace_fmt(...) "failed formatting a trace"
ErrDecl trace_fmt(Str *str, const Trace trace);
void trace_free(Trace *trace);


#define TRACE_H
#endif

