#include "trace.h"
#include <execinfo.h>
#include <stdlib.h>

ErrImpl trace_grab(Trace *trace) {
    const char *msg = 0;
    Trace fatal_trace = {0};
    Str fatal_str = {0};
    int err = 0;
    if(!trace) goto error_null_arg;
    if(memcmp(trace, &(Trace){0}, sizeof(*trace))) goto error_already_initialized;
    /* grab a trace */
    trace->length = backtrace(trace->array, TRACE_DEPTH);
    trace->strings = backtrace_symbols(trace->array, trace->length);
    if(!trace->strings) goto error_strings_failed;

clean:
    trace_free(&fatal_trace);
    str_free(&fatal_str);
    return 0;

    /* error cases - set messges */
error_null_arg:
    msg = "'trace' argument is null!"; goto error;
error_already_initialized:
    msg = "'trace' argument already initialized!"; goto error;
error_strings_failed:
    msg = "'trace' could not get strings"; goto error;

    /* now grab a new empty stack frame and print */
error:
    if(err) ABORT("FATAL ERROR - unrecoverable");
    err = -1;
    TRYC(trace_grab(&fatal_trace));
    TRYC(trace_fmt(&fatal_str, fatal_trace));
    fprintf(stderr, F("--- %s ---", BG_RD FG_BK BOLD) "\n%.*s\n" F("(end of trace)", IT), msg, STR_F(fatal_str));
    goto clean;
}

ErrImpl trace_fmt(Str *str, const Trace trace) {
    int err = 0;
    ASSERT_ARG(str);
    TRYC(str_fmt(str, F("Obtained %d stack frames:\n", BOLD), trace.length));
    for(size_t i = 0; i < trace.length; ++i) {
        TRYC(str_fmt(str, "%3zu: %s\n", trace.length - i - 1, trace.strings[i]));
    }
    TRYC(str_fmt(str, "  -: " F("(end of trace)\n", IT)));
clean:
    return err;
error:
    ERR_CLEAN;
}

void trace_free(Trace *trace) {
    ASSERT_ARG(trace);
    free(trace->strings);
    memset(trace, 0, sizeof(*trace));
}


