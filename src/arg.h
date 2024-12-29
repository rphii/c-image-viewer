#ifndef ARG_H

#include <stdbool.h>
#include "str.h"

#include "vec.h"
VEC_INCLUDE(VBool, vbool, bool, BY_VAL, BASE);
VEC_INCLUDE(VBool, vbool, bool, BY_VAL, ERR);
VEC_INCLUDE(VSize, vsize, bool, BY_VAL, BASE);
VEC_INCLUDE(VSize, vsize, bool, BY_VAL, ERR);
VEC_INCLUDE(VDouble, vdouble, bool, BY_VAL, BASE);
VEC_INCLUDE(VDouble, vdouble, bool, BY_VAL, ERR);

#include "err.h"
#include "lut.h"

/* some characters
 🮮╴╴≎╴┌╴┍ ┬
 └
 U+2237
 ∿
 Bottom Tortoise Shell Bracket

U+25B7

▷
∷
Black Diamond with Down Arrow

U+29EB

⧫
*/
#define ARG_ART_TOP     "∇ "    
#define ARG_ART_LINE    "│ "
#define ARG_ART_TSHAPE  "├╴"
#define ARG_ART_BOT     "╰╴"    

//#include "vector.h"

typedef enum {
    ARG_NONE,       // takes no arguments
    ARG_BOOL,       // takes a boolean value
    ARG_INT,        // takes an integer value
    ARG_FLOAT,      // takes a float value
    ARG_STRING,     // takes a string value
    ARG_OPTION,     // takes only options
    //ARG_ARRAY,      // takes multiple strings
} ArgList;

RStr arg_list(ArgList id);

typedef union ArgVal {
    /* normal / array */
    bool b;     VBool vb;
    ssize_t z;  VSize vz;
    double f;   VDouble vf;
    RStr s;     VrStr vs;
} ArgVal;

typedef union ArgNum {
    ssize_t z;
    double f;
} ArgNum;

typedef int (*ArgFunc)(void *);

//LUT_INCLUDE(TrStr, targval, ArgVal, BY_REF, void *, BY_VAL);

typedef struct ArgObj ArgObj;

typedef struct ArgOpt {
    ArgList id;                 // id of the value to be expected
    ArgVal *value;              // parsed value
    size_t consumed;            // if the parser already consumed an argument of this id
    ArgVal *fallback;           // optional pointer to fallback value. if positional then having a fallback marks it optional
    struct {
        ArgFunc function;       // function to call
        void *data;             // data to function
    } action;                   // calls: action.function(action.data)
    RStr str;                   // switch: multi-character (long)
    RStr help;                  // help text to be displayed
    unsigned char c;            // switch: single-character (short)
    size_t ninput;              // 1 = single value / 0 or >1 = allow multiple values (vector)
    size_t index;               // self-index within array
    ArgNum min;                 // minimum value (only for numbers)
    ArgNum max;                 // maximum value (only for numbers)
    ArgObj *options;            // option values (only available for strings)
    bool quit;
} ArgOpt;

#if 0
typedef struct ArgVar {
    ArgList id;
    ArgVal *value;
    ArgVal *fallback;
    RStr str;
    RStr help;
    ArgNum min;
    ArgNum max;
} ArgVar;
#endif

void argopt_free(ArgOpt *opt);
LUT_INCLUDE(TArgOpt, targopt, RStr, BY_REF, ArgOpt, BY_REF);

#if 0
void argvar_free(ArgVar *var);
LUT_INCLUDE(TArgVar, targvar, RStr, BY_REF, ArgVar, BY_REF);
#endif

VEC_INCLUDE(VpArgOpt, vpargopt, ArgOpt *, BY_VAL, BASE);
VEC_INCLUDE(VpArgOpt, vpargopt, ArgOpt *, BY_VAL, ERR);

#if 0
VEC_INCLUDE(VpArgVar, vpargvar, ArgVar *, BY_VAL, BASE);
VEC_INCLUDE(VpArgVar, vpargvar, ArgVar *, BY_VAL, ERR);
#endif

typedef struct ArgObj {
    TArgOpt table;
    VpArgOpt array;
    RStr info;
} ArgObj;

#if 0
typedef struct ArgEnv {
    TArgVar table;
    VpArgVar array;
    RStr info;
} ArgEnv;
#endif

typedef struct Arg {
    TArgOptItem *cs[256];       // single-character options
    ArgObj options;             // options
    ArgObj positional;          // positional
    size_t positions_used[2];   // need 2 because separate passes for assigning values & running (arg_parse_run)
    RStr name;                  // name of the program (0-th argument)
    const char *version;        // version string
    RStr url;                   // some url
    bool exit_early;            // exit after parsing?
    //ArgEnv environment;       // environment variables
    ArgObj environment;         // environment variables
    RStr info;
    struct {
        bool allow;
        VrStr vrstr;
        RStr info;
    } rest;
} Arg;

int arg_help(void *arg_v);
int arg_version(void *arg_v);

#define ARG_NO_VALUE        0
#define ARG_NO_DEFAULT      0
#define ARG_NO_CALLBACK     0, 0
#define ARG_NO_DATA         0

#define ERR_arg_positional(...) "failed adding positional argument"
ErrDecl arg_positional(ArgObj *obj, ArgOpt *opt);

#define ERR_arg_attach_help(...) "failed attaching help option"
ErrDecl arg_attach_help(Arg *arg, RStr arg_info, RStr arg_url);

#define ERR_arg_attach_version(...) "failed attaching version option"
ErrDecl arg_attach_version(Arg *arg, unsigned char c, RStr str);

#define ERR_arg_attach(...) "failed attaching argument option"
ErrDecl arg_attach(ArgObj *obj, ArgOpt *opt, Arg *arg_if_c_is_set);

#define ERR_argopt_attach_option(...) "failed attaching argument option"
ErrDecl argopt_attach_option(ArgOpt *arg, ArgOpt *opt);

#define ERR_arg_parse_val(...) "failed parsing value"
ErrDecl arg_parse_val(Arg *arg, size_t argc, const char **argv, ArgOpt *opt, RStr arg_rest, size_t *i, bool *deep_error, bool run);

#define ERR_arg_defaults(...) "failed setting default arguments"
ErrDecl arg_defaults(ArgObj *arg);

#define ERR_arg_verify_rec(...) "failed verification of argument settings (rec)"
ErrDecl arg_verify_rec(Arg *arg, ArgObj *obj, bool option_has_optional_data);

#define ERR_arg_verify(...)     "failed verification of argument settings"
ErrDecl arg_verify(Arg *arg);

#define ERR_arg_parse(...) "failed parsing arguments"
ErrDecl arg_parse(Arg *arg, size_t argc, const char **argv);

void arg_allow_rest(Arg *arg, RStr info); // TODO max count ?

ArgOpt *argopt_new(Arg *arg, ArgObj *obj, unsigned char c, RStr str, RStr help);

ArgOpt *arg_option(ArgOpt *opt, ssize_t *picked);
ArgOpt *arg_voption(ArgOpt *opt, VSize *picked, size_t ninput);

void arg_int   (ArgOpt *opt, ssize_t *value, ssize_t *fallback, bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max);
void arg_vint  (ArgOpt *opt, VSize   *value, VSize   *fallback, bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max, size_t ninput);
void arg_float (ArgOpt *opt, double  *value, double  *fallback, bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max);
void arg_vfloat(ArgOpt *opt, VDouble *value, VDouble *fallback, bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max, size_t ninput);
void arg_str   (ArgOpt *opt, RStr    *value, RStr    *fallback, bool quit, ArgFunc function, void *data);
void arg_vstr  (ArgOpt *opt, VrStr   *value, VrStr   *fallback, bool quit, ArgFunc function, void *data, size_t ninput);
void arg_bool  (ArgOpt *opt, bool    *value, bool    *fallback, bool quit, ArgFunc function, void *data);
void arg_vbool (ArgOpt *opt, VBool   *value, VBool   *fallback, bool quit, ArgFunc function, void *data, size_t ninput);
void arg_anon  (ArgOpt *opt, bool quit, ArgFunc function, void  *data);
void arg_enum  (ArgOpt *opt, bool quit, ArgFunc function, void  *data, size_t val);

void arg_free  (Arg *arg);

#define ARG_H
#endif

