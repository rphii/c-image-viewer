#ifndef ARG_H

#include <stddef.h>
#include "lut.h"

#include "str.h"
#include "vec.h"

typedef void *(*ArgCallback)(void *);

typedef enum {
    ARG_NONE,
    ARG_STRING,
    ARG_INTEGER,
    ARG_DOUBLE,
} ArgList;

typedef struct ArgVal {
    union {
        RStr *s;
        int *i;
        double *d;
    };
    bool dynamic;
} ArgVal;

#if 0
VEC_INCLUDE(VrStr, vrstr, const char *, BY_VAL, BASE);
VEC_INCLUDE(VStr, vstr, char *, BY_VAL, BASE);
#endif

typedef struct ArgOpt {
    ArgList id;
    ArgVal val;
    ArgVal ref;
    RStr opt;
    RStr help;
    unsigned char c;
    struct {
        ArgCallback function;
        void *data;
        bool exit_early;
    } callback;
} ArgOpt;

typedef struct ArgOptRefVal {
    ArgOpt *val;
} ArgOptRefVal;

LUT_INCLUDE(TArgOpt, targopt, RStr, BY_REF, ArgOpt, BY_REF);

typedef struct Arg {
    TArgOpt positional;
    TArgOpt options;
    TArgOpt env;
    ArgOpt *c[256];
    RStr name;
    RStr info;
    RStr url;
    struct {
        bool allow;
        VrStr vstr;
        RStr opt;
    } rest;
    int argc;
    const char **argv;
} Arg;

void arg_defaults(Arg *arg);
void arg_attach_help(Arg *arg, unsigned char c, const char *opt, const char *help, const char *name, const char *info, const char *url);
void arg_allow_rest(Arg *arg, const char *opt);
ArgOpt *arg_attach(Arg *arg, TArgOpt *type, unsigned char c, const char *opt, const char *help);
void argopt_set_str(ArgOpt *opt, RStr *ref, RStr *val) __nonnull ((3));
void argopt_set_int(ArgOpt *opt, int *ref, int *val) __nonnull ((3));
void argopt_set_dbl(ArgOpt *opt, double *ref, double *val) __nonnull ((3));
void argopt_set_callback(ArgOpt *opt, ArgCallback callback, void *data, bool exit_early);
int arg_parse(Arg *arg, const int argc, const char **argv, bool *exit_early);

#define ARG_H
#endif

