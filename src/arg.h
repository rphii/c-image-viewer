#ifndef ARG_H

#include <stddef.h>
#include "lut.h"
#include "vec.h"

typedef void *(*ArgCallback)(void *);

typedef enum {
    ARG_NONE,
    ARG_STRING,
    ARG_INTEGER,
    ARG_DOUBLE,
} ArgList;

typedef union ArgVal {
    union {
        char **s;
        int *i;
        double *d;
    };
} ArgVal;

VEC_INCLUDE(VrStr, vrstr, const char *, BY_VAL, BASE);

typedef struct ArgOpt {
    ArgList id;
    ArgVal val;
    ArgVal ref;
    const char *opt;
    const char *help;
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

LUT_INCLUDE(TArgOpt, targopt, char *, BY_VAL, ArgOpt, BY_REF);

typedef struct Arg {
    TArgOpt positional;
    TArgOpt options;
    TArgOpt env;
    ArgOpt *c[256];
    const char *name;
    const char *info;
    const char *url;
    struct {
        bool allow;
        VrStr vstr;
        const char *opt;
    } rest;
    int argc;
    const char **argv;
} Arg;

void arg_defaults(Arg *arg);
void arg_attach_help(Arg *arg, unsigned char c, const char *opt, const char *help, const char *name, const char *info, const char *url);
void arg_allow_rest(Arg *arg, const char *opt);
ArgOpt *arg_attach(Arg *arg, TArgOpt *type, unsigned char c, const char *opt, const char *help);
void argopt_set_str(ArgOpt *opt, char **ref, char **val) __nonnull ((3));
void argopt_set_int(ArgOpt *opt, int *ref, int *val) __nonnull ((3));
void argopt_set_dbl(ArgOpt *opt, double *ref, double *val) __nonnull ((3));
void argopt_set_callback(ArgOpt *opt, ArgCallback callback, void *data, bool exit_early);
int arg_parse(Arg *arg, const int argc, const char **argv, bool *exit_early);

#define ARG_H
#endif

